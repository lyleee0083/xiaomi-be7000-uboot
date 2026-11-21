"""Compile the production ART routines against a NAND test double."""
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "board/qca/arm/common/macaddr_modify.c"
PREFIX = r"""
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define MACADDR_WIFI_MAX 6
#define CONFIG_NAND_FLASH_INFO_IDX 0
#define ARCH_DMA_MINALIGN 64
#define SMEM_BOOT_NAND_FLASH 11
#define SMEM_BOOT_QSPI_NAND_FLASH 12
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BLOCK 0x20000u
#define START (2 * BLOCK)
#define IMAGE_SIZE (4 * BLOCK)

struct mtd_info { u64 size; u32 erasesize; };
struct erase_info { struct mtd_info *mtd; u64 addr, len; };
static struct mtd_info nand_info[] = {{ START + IMAGE_SIZE, BLOCK }};
static struct {
	u32 flash_type, flash_block_size;
} qca_smem_flash_info = { SMEM_BOOT_NAND_FLASH, BLOCK };

static u8 flash[IMAGE_SIZE], image[IMAGE_SIZE], initial[IMAGE_SIZE];
static int writes, erases, report_bad, report_ecc, fault_block, fault;
enum { NO_FAULT, ERASE_FAIL, WRITE_FAIL, SHORT_WRITE, READBACK_FAIL };

static void *fake_memalign(size_t alignment, size_t size)
{
	assert(alignment == ARCH_DMA_MINALIGN);
	return malloc(size);
}
#define memalign fake_memalign

static int smem_getpart(const char *name, u32 *start, u32 *blocks)
{
	assert(!strcmp(name, "0:ART"));
	*start = START / BLOCK;
	*blocks = IMAGE_SIZE / BLOCK;
	return 0;
}

static u64 block_index(const struct mtd_info *mtd, u64 address, size_t length)
{
	assert(mtd == nand_info && address >= START);
	assert(address <= mtd->size && length <= mtd->size - address);
	return address - START;
}

static int mtd_block_isbad(struct mtd_info *mtd, u64 address)
{
	u64 offset = block_index(mtd, address, BLOCK);
	assert(offset % BLOCK == 0);
	return (int)(offset / BLOCK) == report_bad;
}

static int mtd_read(struct mtd_info *mtd, u64 address, size_t length, size_t *read, void *buffer)
{
	u64 offset = block_index(mtd, address, length);
	*read = length;
	memcpy(buffer, flash + offset, length);
	if ((int)(offset / BLOCK) == report_ecc)
		return -EBADMSG;
	if (fault == READBACK_FAIL && (int)(offset / BLOCK) == fault_block &&
	    !memcmp(flash + offset, image + offset, length)) {
		((u8 *)buffer)[0] ^= 1;
		fault = NO_FAULT;
	}
	return 0;
}

static int mtd_erase(struct mtd_info *mtd, struct erase_info *erase)
{
	u64 offset = block_index(mtd, erase->addr, erase->len);
	assert(erase->mtd == mtd && erase->len == BLOCK);
	erases++;
	memset(flash + offset, 0xff, BLOCK);
	if (fault == ERASE_FAIL && (int)(offset / BLOCK) == fault_block) {
		fault = NO_FAULT;
		return -EIO;
	}
	return 0;
}

static int mtd_write(struct mtd_info *mtd, u64 address, size_t length, size_t *done, const void *data)
{
	u64 offset = block_index(mtd, address, length);

	assert(length == BLOCK);
	writes++;
	if ((fault == WRITE_FAIL || fault == SHORT_WRITE) && (int)(offset / BLOCK) == fault_block) {
		int short_write = fault == SHORT_WRITE;
		*done = length / 2;
		memcpy(flash + offset, data, *done);
		fault = NO_FAULT;
		return short_write ? 0 : -EIO;
	}
	memcpy(flash + offset, data, length);
	if ((int)(offset / BLOCK) == report_ecc)
		report_ecc = -1;
	*done = length;
	return 0;
}

static int mtd_is_bitflip(int code) { return code == -EUCLEAN; }
static int mtd_is_eccerr(int code) { return code == -EBADMSG; }

static u16 get_unaligned_le16(const void *value)
{
	const u8 *bytes = value;
	return bytes[0] | (bytes[1] << 8);
}

static int is_valid_ethaddr(const u8 *bytes)
{
	return !(bytes[0] & 1) && (bytes[0] || bytes[1] || bytes[2] ||
				  bytes[3] || bytes[4] || bytes[5]);
}
"""

CASES = r"""
static void init_flash(void)
{
	memset(initial, 0x84, sizeof(initial));
	memcpy(flash, initial, sizeof(flash));
	report_bad = report_ecc = fault_block = -1;
	fault = NO_FAULT;
	writes = erases = 0;
}

static void init_image(void)
{
	size_t i;
	memset(image, 0x13, sizeof(image));
	image[0] = 2;
	for (i = 0; i < ARRAY_SIZE(wifi_layouts); i++) {
		const struct art_wifi_layout *layout = &wifi_layouts[i];
		u8 *bytes = image + layout->offset;
		u16 adjust;

		bytes[0] = ART_WIFI_VALID_HEADER;
		adjust = xor16_checksum(bytes, layout->checksum_size) ^ 0xffff;
		bytes[layout->checksum_offset] ^= adjust;
		bytes[layout->checksum_offset + 1] ^= adjust >> 8;
	}
}

int main(void)
{
	int stage;
	u8 ignored;
	static const u8 mac[] = { 2, 1, 2, 3, 4, 5 };

	init_image();
	init_flash();
	assert(macaddr_restore_art_image(image, sizeof(image)) == 0);
	assert(writes == 4 && erases == 4 && !memcmp(image, flash, sizeof(image)));
	assert(macaddr_restore_art_image(image, sizeof(image)) == 0 && writes == 4);

	init_flash();
	report_bad = 3;
	assert(macaddr_restore_art_image(image, sizeof(image)) == -EIO);
	assert(!writes && !erases && !memcmp(initial, flash, sizeof(flash)));

	init_flash();
	report_ecc = 2;
	assert(macaddr_restore_art_image(image, sizeof(image)) == 0);
	assert(writes == 4 && !memcmp(image, flash, sizeof(image)));

	for (stage = ERASE_FAIL; stage <= READBACK_FAIL; stage++) {
		init_flash();
		fault = stage;
		fault_block = 1;
		assert(macaddr_restore_art_image(image, sizeof(image)) == -EIO);
		assert(!memcmp(initial, flash, sizeof(flash)));

		init_flash();
		fault = stage == READBACK_FAIL ? WRITE_FAIL : stage;
		fault_block = 0;
		assert(art_write(0, sizeof(mac), mac) == -EIO);
		assert(!memcmp(initial, flash, sizeof(flash)));
	}
	init_flash();
	assert(art_write(0, sizeof(mac), mac) == 0);
	assert(!memcmp(flash, mac, sizeof(mac)));
	assert(!memcmp(flash + sizeof(mac), initial + sizeof(mac), sizeof(flash) - sizeof(mac)));
	assert(art_read(0, 1, &ignored) == 0 && ignored == mac[0]);
	puts("ART rollback checks passed.");
	return 0;
}
"""


def main():
    code = SOURCE.read_text(encoding="utf-8")
    art = code[code.index("#define ART_BASE_SIZE"):code.index("int macaddr_read_base(")]
    with tempfile.TemporaryDirectory(prefix="be7000-art-") as directory:
        build = Path(directory)
        harness = build / "test-art.c"
        binary = build / "test-art"
        harness.write_text(PREFIX + art + CASES, encoding="utf-8")
        subprocess.run(
            ["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", str(harness), "-o", str(binary)],
            check=True,
        )
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
