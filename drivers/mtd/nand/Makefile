#
# (C) Copyright 2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

ifdef CONFIG_SPL_BUILD

ifdef CONFIG_SPL_NAND_DRIVERS
NORMAL_DRIVERS=y
endif

obj-$(CONFIG_SPL_NAND_ECC) += nand_ecc.o
obj-$(CONFIG_SPL_NAND_BASE) += nand_base.o
obj-$(CONFIG_SPL_NAND_INIT) += nand.o
ifeq ($(CONFIG_SPL_ENV_SUPPORT),y)
obj-$(CONFIG_ENV_IS_IN_NAND) += nand_util.o
endif

else # not spl

NORMAL_DRIVERS=y

obj-y += nand.o
obj-y += nand_bbt.o
obj-y += nand_ids.o
obj-y += nand_util.o
obj-y += nand_ecc.o
obj-y += nand_base.o
obj-y += nand_timings.o

endif # not spl

ifdef NORMAL_DRIVERS


obj-$(CONFIG_NAND_SPEAR) += spr_nand.o
obj-$(CONFIG_QPIC_NAND) += qpic_nand.o

else  # minimal SPL drivers


endif # drivers
