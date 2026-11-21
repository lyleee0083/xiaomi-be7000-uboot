ifdef CONFIG_LWIP_HTTPD

LWIPDIR = $(srctree)/failsafe/lwip

LWIP_INC = -I$(srctree)/failsafe \
	-isystem $(srctree)/failsafe/arch \
	-I$(srctree)/failsafe/lwip/include \
	-fno-strict-aliasing

MBEDTLS_DIR = $(srctree)/failsafe/mbedtls-source
MBEDTLS_LIBRARY = $(wildcard $(MBEDTLS_DIR)/library/*.c)
ccflags-y += $(LWIP_INC) -DCONFIG_USE_STDINT -I$(srctree)/failsafe/tls \
	-I$(MBEDTLS_DIR)/include -DMBEDTLS_CONFIG_FILE='"be7000_mbedtls_config.h"'

obj-y += lwip/core/altcp.o
obj-y += lwip/core/altcp_alloc.o
obj-y += lwip/core/altcp_tcp.o
obj-y += lwip/apps/altcp_tls/altcp_tls_mbedtls.o
obj-y += lwip/apps/altcp_tls/altcp_tls_mbedtls_mem.o
obj-y += tls/credentials.o tls/recovery_tls.o
obj-y += $(patsubst $(MBEDTLS_DIR)/library/%.c,mbedtls-source/library/%.o,$(MBEDTLS_LIBRARY))

obj-y += failsafe_httpd.o
obj-y += ethernetif.o
obj-y += fs.o
obj-y += lwip/core/init.o
obj-y += lwip/core/def.o
obj-y += lwip/core/mem.o
obj-y += lwip/core/memp.o
obj-y += lwip/core/pbuf.o
obj-y += lwip/core/netif.o
obj-y += lwip/core/ip.o
obj-y += lwip/core/ipv4/ip4.o
obj-y += lwip/core/ipv4/ip4_addr.o
obj-y += lwip/core/ipv4/etharp.o
obj-y += lwip/core/inet_chksum.o
obj-y += lwip/core/tcp.o
obj-y += lwip/core/tcp_in.o
obj-y += lwip/core/tcp_out.o
obj-y += lwip/core/timeouts.o
obj-y += lwip/netif/ethernet.o

endif