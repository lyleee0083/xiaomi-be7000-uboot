"""Check the production HTTP callbacks with simulated independent connections."""
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
PREFIX = r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long ulong;
typedef uint64_t u64;
typedef uint32_t u32_t;
typedef uint16_t u16_t;
typedef uint8_t u8_t;
typedef int err_t;

#define ERR_OK 0
#define ERR_ABRT -1
#define STATE_FILE_REQUEST 1
#define STATE_UPLOAD_REQUEST 2
#define CONFIG_SYS_SDRAM_END 8192u
#define WEBFAILSAFE_UPLOAD_RAM_ADDRESS 4096u
#define CMD_RET_SUCCESS 0
#define min_t(type, a, b) ((type)(a) < (type)(b) ? (type)(a) : (type)(b))
#define mib_int(v) ((unsigned long long)(v) / 1048576)
#define mib_frac(v) (((unsigned long long)(v) % 1048576) * 100 / 1048576)

struct tcp_pcb { int unacked, unsent; };
struct failsafe_httpd_state {
	int state, upgrade_on_ack;
	u32_t last_activity, upload;
	u8_t *dataptr;
	struct tcp_pcb *pcb;
};

static struct {
	u32_t data_size, data_addr;
	int sending_header, chunked, chunk_busy, raw, chunk_num, total_chunks;
	u64 total_remaining, total_sent, total_size, chunk_offset;
	char part_name[64];
} backup;

static int backup_reading, upgrade_in_progress, webfailsafe_ready_for_upgrade;
static int sends, closes, aborts, disconnect_during_read, read_error;
static struct failsafe_httpd_state *hs_global;

static int httpd_transfer_busy(void);

static ulong get_timer(ulong start) { return 1234 - start; }

static void httpd_send_data(struct failsafe_httpd_state *hs)
{
	assert(hs->state == STATE_FILE_REQUEST);
	sends++;
	hs->upload = 0;
}

static err_t httpd_conn_close(struct failsafe_httpd_state *hs, struct tcp_pcb *pcb)
{
	assert(hs->pcb == pcb);
	if (hs == hs_global)
		hs_global = NULL;
	closes++;
	return ERR_OK;
}

static void httpd_conn_abort(struct failsafe_httpd_state *hs, struct tcp_pcb *pcb)
{
	httpd_conn_close(hs, pcb);
	aborts++;
}

static int flashread_partition_chunk(const char *name, ulong addr, u64 offset,
				    ulong size, int raw, ulong *out_offset,
				    ulong *rd_size, char *detail)
{
	assert(name != backup.part_name && !strcmp(name, "fixture"));
	assert(addr == WEBFAILSAFE_UPLOAD_RAM_ADDRESS && size == 48);
	assert(offset == 80 && !raw && !out_offset && detail[0] == '\0');
	assert(httpd_transfer_busy());
	if (disconnect_during_read) {
		httpd_conn_close(hs_global, hs_global->pcb);
		memset(&backup, 0, sizeof(backup));
		assert(httpd_transfer_busy());
		assert(!strcmp(name, "fixture"));
	}
	if (read_error)
		return 1;
	*rd_size = size;
	return CMD_RET_SUCCESS;
}
"""

CASES = r"""
int main(void)
{
	struct tcp_pcb first = {0}, second = {0};
	struct failsafe_httpd_state owner = {.state = STATE_FILE_REQUEST, .pcb = &first};
	struct failsafe_httpd_state terminal = {.state = STATE_FILE_REQUEST, .pcb = &second};

	hs_global = &owner;
	backup.sending_header = backup.chunked = 1;
	backup.data_size = 80;
	backup.total_remaining = 48;
	assert(httpd_sent(&terminal, &second, 20) == ERR_OK);
	assert(closes == 1 && sends == 0 && backup.sending_header && !backup.chunk_busy);
	assert(hs_global == &owner);
	first.unacked = 1;
	assert(httpd_sent(&owner, &first, 20) == ERR_OK && sends == 0);
	first.unacked = 0;
	assert(httpd_sent(&owner, &first, 20) == ERR_OK && sends == 1);
	assert(!backup.sending_header && !backup.chunk_busy);
	assert(httpd_sent(&owner, &first, 20) == ERR_OK && backup.chunk_busy);
	assert(httpd_sent(&terminal, &second, 20) == ERR_OK && closes == 2);
	strcpy(backup.part_name, "fixture");
	backup.chunk_offset = backup.total_sent = 80;
	backup.total_size = 128;
	backup_chunk_next();
	assert(sends == 2 && backup.total_sent == 128 && !backup.total_remaining);
	assert(!backup.chunk_busy && !backup_reading);
	assert(httpd_sent(&owner, &first, 20) == ERR_OK && !hs_global);

	memset(&backup, 0, sizeof(backup));
	hs_global = &owner;
	owner.upgrade_on_ack = 1;
	assert(httpd_sent(&terminal, &second, 20) == ERR_OK && owner.upgrade_on_ack);
	assert(!webfailsafe_ready_for_upgrade && httpd_transfer_busy());
	first.unsent = 1;
	assert(httpd_sent(&owner, &first, 20) == ERR_OK && !webfailsafe_ready_for_upgrade);
	first.unsent = 0;
	assert(httpd_sent(&owner, &first, 20) == ERR_OK && webfailsafe_ready_for_upgrade);
	assert(!hs_global && httpd_transfer_busy());
	webfailsafe_ready_for_upgrade = 0;
	upgrade_in_progress = 1;
	assert(httpd_transfer_busy());
	upgrade_in_progress = 0;
	assert(!httpd_transfer_busy());

	owner.state = STATE_UPLOAD_REQUEST;
	owner.upload = 48;
	assert(httpd_poll_cb(&owner, &first) == ERR_OK && sends == 2);
	owner.state = STATE_FILE_REQUEST;
	owner.upload = 0;

	hs_global = &owner;
	strcpy(backup.part_name, "fixture");
	backup.chunk_offset = 80;
	backup.total_remaining = 48;
	read_error = 1;
	backup_chunk_next();
	assert(aborts == 1 && !hs_global && !httpd_transfer_busy());

	hs_global = &owner;
	disconnect_during_read = 1;
	read_error = 0;
	backup_chunk_next();
	assert(sends == 2 && !hs_global && !httpd_transfer_busy());
	puts("Webfailsafe transfer ownership and response ACK checks passed.");
	return 0;
}
"""


def main():
    code = (ROOT / "failsafe/failsafe_httpd.c").read_text(encoding="utf-8")
    spans = [
        ("static int httpd_transfer_busy(void)", "static void httpd_poll_wait(int count);"),
        ("static void backup_chunk_next(void)", "static void httpd_poll_wait(int count) {"),
        ("static err_t httpd_sent(", "static err_t httpd_recv("),
        ("static err_t httpd_poll_cb(", "static err_t httpd_accept("),
    ]
    production = "\n".join(code[code.index(start):code.index(end)] for start, end in spans)
    with tempfile.TemporaryDirectory(prefix="be7000-httpd-") as tmp:
        source = Path(tmp) / "test.c"
        binary = Path(tmp) / "test"
        source.write_text(PREFIX + production + CASES, encoding="utf-8")
        subprocess.run(
            ["cc", "-std=c99", "-Wall", "-Wextra", "-Wno-unused-parameter",
             "-Werror", str(source), "-o", str(binary)],
            check=True,
        )
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
