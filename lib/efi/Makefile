#
# (C) Copyright 2015 Google, Inc
#
# SPDX-License-Identifier:	GPL-2.0+
#


CFLAGS_REMOVE_efi_stub.o := -mregparm=3 \
	$(if $(CONFIG_EFI_STUB_64BIT),-march=i386 -m32)
CFLAGS_efi_stub.o := -fpic -fshort-wchar
CFLAGS_REMOVE_efi.o := -mregparm=3 \
	$(if $(CONFIG_EFI_STUB_64BIT),-march=i386 -m32)
CFLAGS_efi.o := -fpic -fshort-wchar

