"""Check physical offsets, bad blocks and errors in the NAND backup path."""
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
PREFIX = r"""
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned char u_char;

#define CONFIG_CMD_NAND 1
#define CONFIG_NAND_FLASH_INFO_IDX 0
#define SMEM_BOOT_NAND_FLASH 1
#define SMEM_BOOT_QSPI_NAND_FLASH 2
#define SMEM_BOOT_SPI_FLASH 3
#define CMD_RET_SUCCESS 0
#define CMD_RET_FAILURE 1
#define min_t(type, a, b) ((type)(a) < (type)(b) ? (type)(a) : (type)(b))

typedef struct {
	int flash_type, flash_secondary_type;
	u32 flash_block_size;
} qca_smem_flash_info_t;
struct mtd_info { u64 size; u32 erasesize; };

static qca_smem_flash_info_t qca_smem_flash_info = {SMEM_BOOT_NAND_FLASH, 0, 16};
static struct mtd_info nand_info[] = {{64, 16}};
static int flash_type_new = -1;
static int bad_block = -1, check_error, short_read, yields;
static u_char output[64];

static void flash_yield(void) { yields++; }
static void (*flashread_yield_fn)(void) = flash_yield;

static int smem_getpart(char *name, u32 *start, u32 *size)
{
	if (strcmp(name, "fixture")) return -1;
	*start = 1;
	*size = 2;
	return 0;
}

static int nand_block_isbad(struct mtd_info *mtd, u64 offset)
{
	assert(offset % mtd->erasesize == 0 && offset < mtd->size);
	return check_error ? -EIO : (int)(offset / mtd->erasesize) == bad_block;
}

static int nand_read(struct mtd_info *mtd, u64 offset, size_t *length, u_char *dest)
{
	size_t i;
	assert(offset + *length <= mtd->size);
	if (short_read) (*length)--;
	for (i = 0; i < *length; ++i)
		dest[i] = (u_char)(offset + i);
	return -EUCLEAN;
}

static int nand_raw_chunk_read(u32 addr, u64 offset, u32 size, u32 *out_offset,
			      u32 *out_size, char *out_detail)
{
	assert(!"raw branch unexpectedly selected");
	return CMD_RET_FAILURE;
}
static int get_which_flash_param(char *name) { return 0; }
static int sf_chunk_read(const char *name, u32 addr, u64 offset, u32 size, int raw,
			u32 *out_offset, u32 *out_size, char *out_detail)
{
	assert(!"serial flash branch unexpectedly selected");
	return CMD_RET_FAILURE;
}
"""

CASES = r"""
int main(void)
{
	uintptr_t addr = (uintptr_t)output;
	u32 count, offset;
	int i;

	assert(addr == (u32)addr);
	memset(output, 0xa5, sizeof(output));
	assert(flashread_partition_chunk("fixture", addr, 3, 20, 0, &offset, &count, NULL) == 0);
	assert(offset == 3 && count == 20 && yields == 2);
	for (i = 0; i < 20; ++i)
		assert(output[i] == 19 + i);

	bad_block = 1;
	memset(output, 0xa5, sizeof(output));
	assert(flashread_partition_chunk("nand_full", addr, 11, 24, 0, NULL, &count, NULL) == 0);
	assert(count == 24);
	for (i = 0; i < 24; ++i)
		assert(output[i] == ((i >= 5 && i < 21) ? 0xff : 11 + i));
	assert(output[24] == 0xa5);

	assert(flashread_partition_chunk("fixture", addr, 30, 16, 0, NULL, &count, NULL) == 0);
	assert(count == 2 && output[0] == 46 && output[1] == 47);
	assert(flashread_partition_chunk("fixture", addr, 33, 16, 0, NULL, &count, NULL) != 0);
	assert(flashread_partition_chunk("missing", addr, 0, 16, 0, NULL, &count, NULL) != 0);
	short_read = 1;
	assert(flashread_partition_chunk("nand_full", addr, 0, 16, 0, NULL, &count, NULL) != 0);
	short_read = 0;
	check_error = 1;
	assert(flashread_partition_chunk("nand_full", addr, 0, 16, 0, NULL, &count, NULL) != 0);
	puts("NAND partition offsets, bad-block padding and read error checks passed.");
	return 0;
}
"""


def main():
    code = (ROOT / "common/cmd_flashread.c").read_text(encoding="utf-8")
    start = code.index("\nint flashread_partition_chunk(", code.index("static int sf_chunk_read("))
    end = code.index("\n}\n", start) + 3
    with tempfile.TemporaryDirectory(prefix="be7000-nand-backup-") as tmp:
        source = Path(tmp) / "test.c"
        binary = Path(tmp) / "test"
        source.write_text(PREFIX + code[start:end] + CASES, encoding="utf-8")
        subprocess.run(
            ["cc", "-std=c99", "-Wall", "-Wextra", "-Wno-unused-parameter",
             "-Werror", "-no-pie", str(source), "-o", str(binary)],
            check=True,
        )
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
