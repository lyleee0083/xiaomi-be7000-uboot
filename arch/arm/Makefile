#
# SPDX-License-Identifier:	GPL-2.0+
#

# Select the ARMv7 instruction set supported by the toolchain.
arch-$(CONFIG_CPU_V7)		=$(call cc-option, -march=armv7-a, -march=armv5)
arch-y := $(arch-y)

PLATFORM_CPPFLAGS += $(arch-y)

head-y := arch/arm/cpu/$(CPU)/start.o

ifeq ($(CONFIG_SPL_BUILD),y)
ifneq ($(CONFIG_SPL_START_S_PATH),)
head-y := $(CONFIG_SPL_START_S_PATH:"%"=%)/start.o
endif
endif

libs-y += arch/arm/cpu/$(CPU)/
libs-y += arch/arm/cpu/
libs-y += arch/arm/lib/
