#
# (C) Copyright 2000-2003
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

extra-y	:= start.o

obj-y	+= cache_v7.o

obj-y	+= cpu.o cp15.o
obj-y	+= syslib.o

ifneq ($(CONFIG_ARMV7_NONSEC),)
endif

ifneq ($(CONFIG_ARMV7_PSCI),)
obj-y	+= psci.o
endif


obj-$(CONFIG_QCA_COMMON) += qca/common/
