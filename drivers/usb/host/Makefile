#
# (C) Copyright 2000-2007
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

ifdef CONFIG_DM_USB
obj-$(CONFIG_CMD_USB) += usb-uclass.o
endif

# ohci

# echi
ifdef CONFIG_MPC512X
else
endif

# xhci
obj-$(CONFIG_USB_XHCI) += xhci.o xhci-mem.o xhci-ring.o
obj-$(CONFIG_USB_XHCI_DWC3) += xhci-dwc3.o
obj-$(CONFIG_USB_XHCI_IPQ) += xhci-ipq.o
# designware
