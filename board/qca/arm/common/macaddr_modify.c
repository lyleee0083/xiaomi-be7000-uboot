#include <common.h>
#include <command.h>
#include <nand.h>
#include <net.h>
#include <asm/errno.h>
#include <asm/unaligned.h>
#include <asm/arch-qca-common/smem.h>
#include <asm/arch-qca-common/qca_common.h>
#include "fdt_info.h"
#include "macaddr_modify.h"

#define ART_BASE_SIZE		0x1000
#define ART_PART_SIZE		0x100000
#define ART_ERASE_SIZE		0x20000
#define ART_MAC_LEN		6
#define ART_WIFI_HEADER_SIZE	28
#define ART_WIFI_VALID_HEADER	1

struct art_wifi_layout {
	u32 offset;
	u32 checksum_size;
	u8 mac_offset;
	u8 checksum_offset;
};

static const struct art_wifi_layout wifi_layouts[MACADDR_WIFI_MAX / 2] = {
	{ 0x01000, 0x20000, 0x0e, 0x0a },
	{ 0x33000, 0x18800, 0x10, 0x0c },
	{ 0x65000, 0x18800, 0x10, 0x0c },
};

static u16 xor16_checksum(const void *buf, size_t len)
{
	const u16 *words = buf;
	u16 checksum = 0;
	size_t i;

	for (i = 0; i < len / 2; i++)
		checksum ^= le16_to_cpu(words[i]);
	return checksum;
}

static loff_t art_part_offset(void)
{
	u32 start_blocks, size_blocks;

	if (smem_getpart("0:ART", &start_blocks, &size_blocks) < 0)
		return -1;

	return (loff_t)qca_smem_flash_info.flash_block_size * start_blocks;
}

static int art_read(loff_t offset, size_t size, void *buf)
{
	struct mtd_info *mtd = &nand_info[CONFIG_NAND_FLASH_INFO_IDX];
	u32 flash_type = qca_smem_flash_info.flash_type;
	size_t read_len;
	loff_t base;
	int ret;

	if (flash_type != SMEM_BOOT_NAND_FLASH &&
	    flash_type != SMEM_BOOT_QSPI_NAND_FLASH)
		return -EINVAL;
	if (offset < 0 || offset > ART_PART_SIZE ||
	    size > ART_PART_SIZE - offset)
		return -EINVAL;

	base = art_part_offset();
	if (base < 0 || base > 0xffffffffLL)
		return -EINVAL;

	ret = mtd_read(mtd, base + offset, size, &read_len, buf);
	if (ret < 0 && !mtd_is_bitflip(ret))
		return ret;

	return read_len == size ? 0 : -EIO;
}

static int art_write(loff_t offset, size_t size, const void *buf)
{
	struct mtd_info *mtd = &nand_info[CONFIG_NAND_FLASH_INFO_IDX];
	struct erase_info erase = { 0 };
	loff_t base, block_offset;
	size_t in_block, written;
	u8 *block;
	int ret;

	if (offset < 0 || offset >= ART_PART_SIZE || !size ||
	    mtd->erasesize != ART_ERASE_SIZE)
		return -EINVAL;

	in_block = (size_t)offset % ART_ERASE_SIZE;
	if (size > ART_ERASE_SIZE - in_block)
		return -EINVAL;
	block_offset = offset - in_block;

	/* Preserve the other ART data in the NAND erase block. */
	block = memalign(ARCH_DMA_MINALIGN, ART_ERASE_SIZE);
	if (!block)
		return -EINVAL;
	ret = art_read(block_offset, ART_ERASE_SIZE, block);
	if (ret < 0)
		goto out;

	memcpy(block + in_block, buf, size);
	base = art_part_offset();
	if (base < 0 || base > 0xffffffffLL) {
		ret = -EINVAL;
		goto out;
	}

	erase.mtd = mtd;
	erase.addr = base + block_offset;
	erase.len = ART_ERASE_SIZE;
	ret = mtd_erase(mtd, &erase);
	if (ret < 0)
		goto out;

	ret = mtd_write(mtd, erase.addr, ART_ERASE_SIZE, &written, block);
	if (ret < 0)
		goto out;
	if (written != ART_ERASE_SIZE) {
		ret = -EIO;
		goto out;
	}

	ret = art_read(block_offset, ART_ERASE_SIZE, block);
	if (!ret && memcmp(block + in_block, buf, size))
		ret = -EIO;
out:
	free(block);
	return ret;
}

int macaddr_read_base(u8 *mac, int mac_index) {
	u8 *buf;
	int ret;

	if (!mac || mac_index < 0 || mac_index >= CONFIG_IPQ_NO_MACS)
		return -EINVAL;

	buf = memalign(ARCH_DMA_MINALIGN, ART_BASE_SIZE);
	if (!buf)
		return -ENOMEM;

	ret = art_read(0, ART_BASE_SIZE, buf);
	if (ret >= 0)
		memcpy(mac, &buf[mac_index * ART_MAC_LEN], ART_MAC_LEN);

	free(buf);
	return ret;
}

int macaddr_modify_base(const u8 *new_mac, int mac_index) {
	u8 *buf;
	int ret;

	if (!new_mac || !is_valid_ethaddr(new_mac) ||
	    mac_index < 0 || mac_index >= CONFIG_IPQ_NO_MACS)
		return -EINVAL;

	buf = memalign(ARCH_DMA_MINALIGN, ART_BASE_SIZE);
	if (!buf)
		return -ENOMEM;

	ret = art_read(0, ART_BASE_SIZE, buf);
	if (ret < 0)
		goto out;

	memcpy(&buf[mac_index * ART_MAC_LEN], new_mac, ART_MAC_LEN);

	ret = art_write(0, ART_BASE_SIZE, buf);
out:
	free(buf);
	return ret;
}

static const struct art_wifi_layout *art_wifi_layout(int index)
{
	if (index < 0 || index >= MACADDR_WIFI_MAX)
		return NULL;
	return &wifi_layouts[index / 2];
}

static size_t art_wifi_mac_offset(const struct art_wifi_layout *layout, int index)
{
	return layout->mac_offset + (index % 2) * ART_MAC_LEN;
}

int macaddr_read_wifi(u8 *mac, int wifi_index)
{
	const struct art_wifi_layout *layout = art_wifi_layout(wifi_index);
	u8 version;
	int ret;

	if (!layout || !mac)
		return -EINVAL;

	ret = art_read(layout->offset, sizeof(version), &version);
	if (ret < 0)
		return ret;
	if (version != ART_WIFI_VALID_HEADER)
		return -EINVAL;

	return art_read(layout->offset + art_wifi_mac_offset(layout, wifi_index),
			ART_MAC_LEN, mac);
}

int macaddr_modify_wifi(const u8 *new_mac, int wifi_index)
{
	const struct art_wifi_layout *layout = art_wifi_layout(wifi_index);
	u8 header[ART_WIFI_HEADER_SIZE];
	u8 *mac, *checksum_field;
	u16 checksum;
	size_t i;
	int ret;

	if (!layout || !new_mac || !is_valid_ethaddr(new_mac) ||
	    !macaddr_wifi_cs_valid(wifi_index))
		return -EINVAL;

	ret = art_read(layout->offset, sizeof(header), header);
	if (ret < 0)
		return ret;

	mac = header + art_wifi_mac_offset(layout, wifi_index);
	checksum_field = header + layout->checksum_offset;
	checksum = get_unaligned_le16(checksum_field);
	/* Replace the old MAC words in the verified XOR16 checksum. */
	for (i = 0; i < ART_MAC_LEN; i += sizeof(u16))
		checksum ^= get_unaligned_le16(mac + i) ^
			    get_unaligned_le16(new_mac + i);

	put_unaligned_le16(checksum, checksum_field);
	memcpy(mac, new_mac, ART_MAC_LEN);
	return art_write(layout->offset, sizeof(header), header);
}

static void print_mac(const char *name, const u8 *mac) {
	printf("%s: %02X:%02X:%02X:%02X:%02X:%02X\n", name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

u32 macaddr_wifi_offset(int wifi_index)
{
	const struct art_wifi_layout *layout = art_wifi_layout(wifi_index);

	if (!layout)
		return 0;
	return layout->offset + art_wifi_mac_offset(layout, wifi_index);
}

u32 macaddr_wifi_cs_offset(int wifi_index)
{
	const struct art_wifi_layout *layout = art_wifi_layout(wifi_index);

	if (!layout)
		return 0;
	return layout->offset + layout->checksum_offset;
}

u16 macaddr_wifi_checksum(int wifi_index)
{
	u8 buf[2];
	u32 offset = macaddr_wifi_cs_offset(wifi_index);

	if (!offset || art_read(offset, sizeof(buf), buf) < 0)
		return 0;
	return (buf[0] << 8) | buf[1];
}

int macaddr_wifi_cs_valid(int wifi_index)
{
	const struct art_wifi_layout *layout = art_wifi_layout(wifi_index);
	u8 header[16], *cal;
	int valid;

	if (!layout || art_read(layout->offset, sizeof(header), header) < 0 ||
	    header[0] != ART_WIFI_VALID_HEADER)
		return 0;

	cal = memalign(ARCH_DMA_MINALIGN, layout->checksum_size);
	if (!cal)
		return 0;

	valid = art_read(layout->offset, layout->checksum_size, cal) == 0 &&
		xor16_checksum(cal, layout->checksum_size) == 0xffff;
	free(cal);
	return valid;
}

static int parse_iface(const char *arg, int *type, int *index) {
	if ((!strncmp(arg, "eth", 3) && arg[3] >= '0' && arg[3] <= '9') ||
	    (!strncmp(arg, "wifi", 4) && arg[4] >= '0' && arg[4] <= '9')) {
		*type = arg[0] == 'e' ? 0 : 1;
		*index = simple_strtoul(arg + (*type ? 4 : 3), NULL, 0);
		return 0;
	}
	return -1;
}

static int do_mac(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]) {
	u8 mac[6];
	char name[8];
	int i, type, index, ret, is_write;

	if (argc == 1) {
		for (i = 0; i < CONFIG_IPQ_NO_MACS; i++) {
			if (macaddr_read_base(mac, i) >= 0) {
				sprintf(name, "eth%d", i);
				print_mac(name, mac);
			}
		}
		for (i = 0; i < MACADDR_WIFI_MAX; i++) {
			if (macaddr_read_wifi(mac, i) >= 0 && is_valid_ethaddr(mac)) {
				sprintf(name, "wifi%d", i);
				print_mac(name, mac);
			}
		}
		return CMD_RET_USAGE;
	}

	if (argc < 3 || parse_iface(argv[2], &type, &index) < 0)
		return CMD_RET_USAGE;

	is_write = strcmp(argv[1], "w") == 0;
	if (!is_write && strcmp(argv[1], "r") != 0)
		return CMD_RET_USAGE;

	if (is_write) {
		if (argc != 4)
			return CMD_RET_USAGE;
		eth_parse_enetaddr(argv[3], mac);
		if (!is_valid_ethaddr(mac)) {
			printf("Invalid MAC address\n");
			return CMD_RET_FAILURE;
		}
	}

	ret = is_write
		? (type == 0 ? macaddr_modify_base(mac, index) : macaddr_modify_wifi(mac, index))
		: (type == 0 ? macaddr_read_base(mac, index) : macaddr_read_wifi(mac, index));
	if (ret < 0) {
		printf("MAC %s failed: %d\n", is_write ? "write" : "read", ret);
		return CMD_RET_FAILURE;
	}

	if (is_write)
		printf("MAC address updated\n");
	else
		print_mac(argv[2], mac);
	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(mac, 4, 0, do_mac,
	"Read/Write MAC address in ART partition",
	"- Read all MAC addresses\n"
	"  r eth|wifi<n>       - Read MAC\n"
	"  w eth|wifi<n> <mac> - Write MAC\n"
#if defined(CONFIG_IPQ40XX) || defined(CONFIG_IPQ806X)
	"wifi0:2.4G wifi1:5G wifi2:QCA988x"
#else
	"wifi0:2.4G wifi1:5G (wifi2/3:2nd wifi4/5:3rd)"
#endif
);