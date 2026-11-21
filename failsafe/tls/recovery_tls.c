/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <common.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "mbedtls/entropy.h"
#include "recovery_tls.h"

/* IPQ9574 TRNG and GCC clock vote from Linux ipq9574.dtsi,
 * gcc-ipq9574.c and qcom-rng.c.
 */
#define TRNG_DATA	0x000e3000
#define TRNG_STATUS	0x000e3004
#define GCC_PRNG_VOTE	0x0180b004
#define GCC_PRNG_CLOCK	0x01813024

#define SLOT_ENABLED	32
#define SLOT_LENGTHS	36
#define SLOT_CONTENT	48

static struct altcp_tls_config *recovery_credentials;

int recovery_tls_is_provisioned(void)
{
	return get_unaligned_le32(be7000_webterm_provisioning + SLOT_ENABLED) != 0;
}

struct altcp_pcb *recovery_tls_new(void)
{
	const unsigned char *slot = be7000_webterm_provisioning;
	const unsigned char *key = slot + SLOT_CONTENT;
	unsigned int key_len = get_unaligned_le32(slot + SLOT_LENGTHS);
	unsigned int server_len = get_unaligned_le32(slot + SLOT_LENGTHS + 4);
	unsigned int ca_len = get_unaligned_le32(slot + SLOT_LENGTHS + 8);
	unsigned int limit = BE7000_TLS_SLOT_SIZE - SLOT_CONTENT;
	const unsigned char *server;
	const unsigned char *ca;

	if (get_unaligned_le32(slot + SLOT_ENABLED) != 1 ||
	    !key_len || !server_len || !ca_len ||
	    key_len > limit || server_len > limit - key_len ||
	    ca_len > limit - key_len - server_len)
		return NULL;

	server = key + key_len;
	ca = server + server_len;
	recovery_credentials = altcp_tls_create_config_server_mutual(
		key, key_len, server, server_len, ca, ca_len);
	if (!recovery_credentials)
		return NULL;
	return altcp_tls_new(recovery_credentials, IPADDR_TYPE_V4);
}

void recovery_tls_release(void)
{
	if (recovery_credentials) {
		altcp_tls_free_config(recovery_credentials);
		recovery_credentials = NULL;
	}
}

int mbedtls_hardware_poll(void *context, unsigned char *output, size_t length,
			  size_t *written)
{
	unsigned long started;
	u32 word;
	size_t copied;

	(void)context;
	*written = 0;
	setbits_le32(GCC_PRNG_VOTE, 1U << 10);
	started = get_timer(0);
	while (readl(GCC_PRNG_CLOCK) & (1U << 31)) {
		if (get_timer(started) >= 100)
			return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
		udelay(10);
	}

	while (*written < length) {
		started = get_timer(0);
		while (!(readl(TRNG_STATUS) & 1)) {
			if (get_timer(started) >= 100)
				return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
			udelay(10);
		}
		word = readl(TRNG_DATA);
		if (!word)
			return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
		copied = min(length - *written, sizeof(word));
		memcpy(output + *written, &word, copied);
		*written += copied;
	}
	return 0;
}
