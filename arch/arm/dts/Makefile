#
# SPDX-License-Identifier:	GPL-2.0+
#

dtb-$(CONFIG_IPQ9574_XIAOMI_BE7000) += ipq9574-xiaomi-be7000.dtb

targets += $(dtb-y)

# Add any required device tree compiler flags here
DTC_FLAGS +=

PHONY += dtbs

ifneq ($(CONFIG_OF_COMBINE),)

%(obj)/%.dtb.comb.S: %(obj)/%.dtb
	@:

$(obj)/dtbtable.S: $(addsuffix .comb.o, $(addprefix $(obj)/, $(dtb-y)))
	@:

obj-y := dtbtable.o $(addsuffix .comb.o, $(dtb-y))

targets += obj-$(CONFIG_OF_EMBED)

else
dtbs: $(addprefix $(obj)/, $(dtb-y))
	@:
endif

clean-files := *.dtb *.o *.S