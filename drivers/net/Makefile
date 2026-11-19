#
# (C) Copyright 2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#

ccflags-y += -I$(srctree)/board/qca/arm/common
ccflags-y += -I$(srctree)/drivers/net/ipq_common

obj-$(CONFIG_FSL_MC_ENET) += fsl-mc/
obj-$(CONFIG_FSL_MC_ENET) += ldpaa_eth/
obj-$(CONFIG_IPQ9574_EDMA) += ipq9574/ipq9574_ppe.o
obj-$(CONFIG_IPQ9574_EDMA) += ipq9574/ipq9574_uniphy.o
obj-$(CONFIG_IPQ9574_EDMA) += ipq9574/ipq9574_edma.o


obj-$(CONFIG_IPQ_MDIO) += ipq_common/ipq_mdio.o
obj-$(CONFIG_IPQ_QTI_BIT_BANGMII) += ipq_common/ipq_bitbangmii.o
obj-$(CONFIG_QCA8084_PHY) += ipq_common/ipq_qca8084.o
obj-$(CONFIG_QCA8084_PHY) += ipq_common/ipq_qca8084_clk.o
obj-$(CONFIG_QCA8084_PHY) += ipq_common/ipq_qca8084_interface_ctrl.o
obj-$(CONFIG_QCA8081_PHY) += ipq_common/ipq_qca8081.o
