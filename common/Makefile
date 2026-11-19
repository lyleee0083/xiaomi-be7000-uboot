#
# (C) Copyright 2004-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

# core
ifndef CONFIG_SPL_BUILD
obj-y += init/
obj-y += main.o
obj-y += exports.o
obj-y += hash.o
ifdef CONFIG_SYS_HUSH_PARSER
obj-y += cli_hush.o
endif

# This option is not just y/n - it can have a numeric value
ifdef CONFIG_BOOTDELAY
obj-y += autoboot.o
endif

# This option is not just y/n - it can have a numeric value
ifdef CONFIG_BOOT_RETRY_TIME
endif

# boards
obj-$(CONFIG_SYS_GENERIC_BOARD) += board_f.o
obj-$(CONFIG_SYS_GENERIC_BOARD) += board_r.o
obj-$(CONFIG_DISPLAY_BOARDINFO) += board_info.o
obj-$(CONFIG_DISPLAY_BOARDINFO_LATE) += board_info.o

# core command
obj-y += cmd_boot.o
obj-$(CONFIG_CMD_BOOTM) += cmd_bootm.o bootm.o bootm_os.o
obj-y += cmd_help.o
obj-y += cmd_version.o

# environment
obj-y += env_attr.o
obj-y += env_callback.o
obj-y += env_flags.o
extra-$(CONFIG_ENV_IS_EMBEDDED) += env_embedded.o
obj-$(CONFIG_ENV_IS_IN_EEPROM) += env_embedded.o
extra-$(CONFIG_ENV_IS_IN_FLASH) += env_embedded.o
obj-$(CONFIG_ENV_IS_IN_NVRAM) += env_embedded.o
obj-$(CONFIG_ENV_IS_IN_NAND) += env_nand.o
obj-$(CONFIG_ENV_IS_IN_SPI_FLASH) += env_sf.o

# command
obj-$(CONFIG_CMD_AES) += cmd_aes.o
obj-$(CONFIG_SOURCE) += cmd_source.o
obj-$(CONFIG_CMD_SOURCE) += cmd_source.o
obj-$(CONFIG_CMD_BDI) += cmd_bdinfo.o
obj-$(CONFIG_CMD_CACHE) += cmd_cache.o
obj-$(CONFIG_CMD_CONSOLE) += cmd_console.o
obj-$(CONFIG_CMD_DM) += cmd_dm.o
ifdef CONFIG_4xx
endif
ifdef CONFIG_POST
endif
obj-$(CONFIG_CMD_ECHO) += cmd_echo.o
obj-$(CONFIG_CMD_ELF) += cmd_elf.o
obj-$(CONFIG_SYS_HUSH_PARSER) += cmd_exit.o
obj-$(CONFIG_CMD_FAT) += cmd_fat.o
ifndef CONFIG_CMD_DISABLE_FDT
obj-y += cmd_fdt.o
endif
obj-$(CONFIG_OF_LIBFDT) += fdt_support.o
obj-$(CONFIG_CMD_FLASH) += cmd_flash.o
obj-$(CONFIG_CMD_FLASHWRITE) += cmd_flashwrite.o
obj-$(CONFIG_CMD_FLASHREAD) += cmd_flashread.o
ifdef CONFIG_FPGA
endif
obj-$(CONFIG_CMD_I2C) += cmd_i2c.o
obj-$(CONFIG_CMD_ITEST) += cmd_itest.o
obj-$(CONFIG_CMD_MEMORY) += cmd_mem.o
obj-$(CONFIG_MII) += miiphyutil.o
obj-$(CONFIG_CMD_MII) += miiphyutil.o
obj-$(CONFIG_PHYLIB) += miiphyutil.o
obj-$(CONFIG_CMD_MII) += cmd_mii.o
ifdef CONFIG_PHYLIB
endif
obj-$(CONFIG_CMD_MISC) += cmd_misc.o
obj-$(CONFIG_CMD_MTDPARTS) += cmd_mtdparts.o
obj-$(CONFIG_CMD_NAND) += cmd_nand.o
obj-$(CONFIG_CMD_NET) += cmd_net.o
obj-$(CONFIG_CMD_PART) += cmd_part.o
ifdef CONFIG_PCI
endif
obj-y += cmd_pcmcia.o
obj-$(CONFIG_CMD_SF) += cmd_sf.o
obj-$(CONFIG_CMD_SETEXPR) += cmd_setexpr.o
obj-$(CONFIG_SYS_HUSH_PARSER) += cmd_test.o
obj-$(CONFIG_CMD_UBI) += cmd_ubi.o
ifdef CONFIG_LZMA
endif
ifdef CONFIG_CMD_USB
obj-y += cmd_usb.o
obj-y += usb.o usb_hub.o
obj-$(CONFIG_USB_STORAGE) += usb_storage.o
endif

obj-$(CONFIG_CMD_XIMG) += cmd_ximg.o
obj-$(CONFIG_CMD_ZIP) += cmd_zip.o

# others
obj-y += flash.o
obj-y += splash.o

# Power
endif

ifdef CONFIG_SPL_BUILD
obj-$(CONFIG_SPL_YMODEM_SUPPORT) += xyzModem.o
obj-$(CONFIG_SPL_NET_SUPPORT) += miiphyutil.o
obj-$(CONFIG_SPL_OF_TRANSLATE) += fdt_support.o
ifdef CONFIG_SPL_USB_HOST_SUPPORT
obj-$(CONFIG_SPL_USB_SUPPORT) += usb.o usb_hub.o
obj-$(CONFIG_USB_STORAGE) += usb_storage.o
endif
ifdef CONFIG_SPL_SATA_SUPPORT
endif
# environment
ifdef CONFIG_SPL_ENV_SUPPORT
obj-$(CONFIG_SPL_ENV_SUPPORT) += env_attr.o
obj-$(CONFIG_SPL_ENV_SUPPORT) += env_flags.o
obj-$(CONFIG_SPL_ENV_SUPPORT) += env_callback.o
obj-$(CONFIG_ENV_IS_IN_NAND) += env_nand.o
obj-$(CONFIG_ENV_IS_IN_SPI_FLASH) += env_sf.o
endif
endif
# core command
obj-y += cmd_nvedit.o
#environment
obj-y += env_common.o
#others
ifdef CONFIG_SPL_BUILD
obj-$(CONFIG_SPL_SERIAL_SUPPORT) += console.o
else
obj-y += console.o
endif
obj-y += dlmalloc.o
ifdef CONFIG_SYS_MALLOC_F_LEN
obj-y += malloc_simple.o
endif
obj-y += image.o
obj-$(CONFIG_OF_LIBFDT) += image-fdt.o
obj-$(CONFIG_FIT) += image-fit.o
obj-y += memsize.o
obj-y += stdio.o
obj-$(CONFIG_LWIP_HTTPD) += webterm.o
CFLAGS_webterm.o += -I$(srctree)/failsafe -I$(srctree)/failsafe/lwip/include -I$(srctree)/failsafe/arch -fno-strict-aliasing

# This option is not just y/n - it can have a numeric value
ifdef CONFIG_FASTBOOT_FLASH
ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
endif
ifdef CONFIG_FASTBOOT_FLASH_NAND_DEV
endif
endif


# We always have this since drivers/ddr/fs/interactive.c needs it
obj-y += cli_simple.o

obj-y += cli.o
obj-y += cli_readline.o
obj-y += command.o
obj-y += s_record.o
ifndef CONFIG_REDUCE_FOOTPRINT
obj-y += xyzModem.o
endif
obj-y += cmd_disk.o

CFLAGS_env_embedded.o := -Wa,--no-warn -DENV_CRC=$(shell tools/envcrc 2>/dev/null)