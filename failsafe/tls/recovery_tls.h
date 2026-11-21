/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef BE7000_RECOVERY_TLS_H
#define BE7000_RECOVERY_TLS_H

struct altcp_pcb;

#define BE7000_TLS_SLOT_SIZE 4096

extern const unsigned char be7000_webterm_provisioning[BE7000_TLS_SLOT_SIZE];
int recovery_tls_is_provisioned(void);
struct altcp_pcb *recovery_tls_new(void);
void recovery_tls_release(void);

#endif
