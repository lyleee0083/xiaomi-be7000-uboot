#
# (C) Copyright 2000-2007
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

ifneq (,$(findstring y,$(CONFIG_MTD_DEVICE)$(CONFIG_CMD_NAND)$(CONFIG_CMD_ONENAND)$(CONFIG_CMD_SF)))
obj-y += mtdcore.o mtd_uboot.o
endif
obj-$(CONFIG_MTD_PARTITIONS) += mtdpart.o
obj-$(CONFIG_QCA_SPI) += ipq_spi_flash.o
