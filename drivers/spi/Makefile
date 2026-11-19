#
# (C) Copyright 2000-2007
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

# There are many options which enable SPI, so make this library available
ifdef CONFIG_DM_SPI
else
obj-y += spi.o
endif

ifdef CONFIG_QCA_SPI
obj-$(CONFIG_ARCH_IPQ40xx) += qca_qup_spi_bam.o
obj-$(CONFIG_ARCH_IPQ807x) += qca_qup_spi_bam.o
obj-$(CONFIG_ARCH_IPQ6018) += qca_qup_spi_bam.o
obj-$(CONFIG_ARCH_IPQ5018) += qca_qup_spi_bam.o
obj-$(CONFIG_ARCH_IPQ5332) += qca_qup_spi_bam.o
obj-$(CONFIG_ARCH_IPQ9574) += qca_qup_spi_bam.o

endif
