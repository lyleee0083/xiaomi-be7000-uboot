#
# SPDX-License-Identifier:	GPL-2.0+
#

X86DIR  = x86emu

obj-y = \
	$(X86DIR)/decode.o $(X86DIR)/ops2.o $(X86DIR)/ops.o $(X86DIR)/prim_ops.o \
	$(X86DIR)/sys.o $(X86DIR)/debug.o

ccflags-y := -I$(srctree)/$(src) -I$(srctree)/$(src)/include \
	$(if $(CONFIG_PPC),-D__PPC__  -D__BIG_ENDIAN__)
