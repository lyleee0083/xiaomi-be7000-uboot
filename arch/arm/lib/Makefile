#
# (C) Copyright 2002-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

lib-$(CONFIG_USE_PRIVATE_LIBGCC) += _ashldi3.o _ashrdi3.o _divsi3.o \
			_lshrdi3.o _modsi3.o _udivsi3.o _umodsi3.o div0.o \
			_uldivmod.o

ifdef CONFIG_CPU_V7M
obj-y += crt0.o
else ifdef CONFIG_ARM64
else
obj-y	+= vectors.o crt0.o
endif

ifndef CONFIG_SPL_BUILD
ifdef CONFIG_ARM64
else
obj-y	+= relocate.o
endif

obj-$(CONFIG_OF_LIBFDT) += bootm-fdt.o
obj-$(CONFIG_CMD_BOOTM) += bootm.o
else
endif

obj-y	+= sections.o
obj-y	+= stack.o
ifdef CONFIG_CPU_V7M
else ifdef CONFIG_ARM64
else
obj-y	+= interrupts.o
endif
ifndef CONFIG_RESET
obj-y	+= reset.o
endif

obj-y	+= cache.o
ifndef CONFIG_ARM64
obj-y	+= cache-cp15.o
endif


# For EABI conformant tool chains, provide eabi_compat()
ifneq (,$(findstring -mabi=aapcs-linux,$(PLATFORM_CPPFLAGS)))
extra-y	+= eabi_compat.o
endif

# some files can only build in ARM or THUMB2, not THUMB1

ifdef CONFIG_SYS_THUMB_BUILD
ifndef CONFIG_HAS_THUMB2

# for C files, just apend -marm, which will override previous -mthumb*

CFLAGS_cache.o := -marm
CFLAGS_cache-cp15.o := -marm

# For .S, drop -mthumb* and other thumb-related options.
# CFLAGS_REMOVE_* would not have an effet, so AFLAGS_REMOVE_*
# was implemented and is used here.
# Also, define ${target}_NO_THUMB_BUILD for these two targets
# so that the code knows it should not use Thumb.

AFLAGS_REMOVE_memset.o := -mthumb -mthumb-interwork
AFLAGS_REMOVE_memcpy.o := -mthumb -mthumb-interwork
AFLAGS_memset.o := -DMEMSET_NO_THUMB_BUILD
AFLAGS_memcpy.o := -DMEMCPY_NO_THUMB_BUILD

endif
endif
