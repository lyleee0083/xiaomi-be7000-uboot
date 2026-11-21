/*
 * webterm.c - Web Terminal for U-Boot
 * Provides console output capture and transfer to HTTP interface
 */

#include <common.h>
#include <malloc.h>
#include <command.h>
#include "../failsafe/failsafe_httpd.h"
#include "../failsafe/failsafe_httpd_types.h"
#include "../failsafe/request.h"
#include <net.h>

#define WEBTERM_BUFFER_SIZE 16384
#define WEBTERM_LINE_SIZE 256
#define WEBTERM_RESPONSE_SIZE 32768
#define WEBTERM_MAX_CMD_LEN 4096
#define WEBTERM_TOKEN_LEN 32
#define WEBTERM_LEASE_MS (10 * 60 * 1000)

struct webterm_buffer {
	char *buffer;
	int size, head, tail, count, overflow;
};

static struct webterm_buffer webterm_out = {0};
static char webterm_line_buffer[WEBTERM_LINE_SIZE];
static int webterm_line_pos = 0;
static int webterm_prev_char_was_cr = 0;

static volatile int webterm_output_seq = 0;

static char webterm_pending_cmd[WEBTERM_MAX_CMD_LEN] = {0};
static volatile int webterm_has_pending_cmd = 0;
volatile int webterm_abort_requested = 0;

static char webterm_response_buf[WEBTERM_RESPONSE_SIZE];
static char webterm_output_buf[WEBTERM_BUFFER_SIZE];
static char webterm_token[WEBTERM_TOKEN_LEN];
static ip4_addr_t webterm_peer;
static ulong webterm_start;
static int webterm_unlocked;

static void webterm_batch_copy(const char *src, int len);
static void webterm_flush_line(void);
static void webterm_add_char(char c);
static void webterm_respond(struct failsafe_httpd_state *hs, int code, const char *ctype, const char *fmt, ...);

int webterm_init(void) {
	if (webterm_out.buffer)
		return 0;

	webterm_out.buffer = malloc(WEBTERM_BUFFER_SIZE);
	if (!webterm_out.buffer)
		return -1;

	webterm_out.size = WEBTERM_BUFFER_SIZE;
	webterm_out.head = 0;
	webterm_out.tail = 0;
	webterm_out.count = 0;
	webterm_out.overflow = 0;
	webterm_line_pos = 0;

	return 0;
}

void webterm_reset(void) {
	if (webterm_out.buffer) {
		webterm_out.head = 0;
		webterm_out.tail = 0;
		webterm_out.count = 0;
		webterm_out.overflow = 0;
	}
	webterm_output_seq = 0;
	webterm_line_pos = 0;
	webterm_prev_char_was_cr = 0;
}

void webterm_lock(void)
{
	webterm_unlocked = 0;
	memset(webterm_token, 0, sizeof(webterm_token));
	memset(webterm_pending_cmd, 0, sizeof(webterm_pending_cmd));
	webterm_has_pending_cmd = 0;
	webterm_abort_requested = 0;
	webterm_reset();
}

static int webterm_active(void)
{
	if (webterm_unlocked && get_timer(webterm_start) >= WEBTERM_LEASE_MS)
		webterm_lock();
	return webterm_unlocked;
}

static int do_webterm_access(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ip4_addr_t peer;
	size_t i;

	if (argc == 2 && !strcmp(argv[1], "lock")) {
		webterm_lock();
		return CMD_RET_SUCCESS;
	}
	if (argc != 4 || strcmp(argv[1], "allow"))
		return CMD_RET_USAGE;
	/* A running recovery session cannot re-arm itself. */
	if (webfailsafe_is_running) {
		printf("Stop Webfailsafe before arming the web terminal from the local console\n");
		return CMD_RET_FAILURE;
	}
	if (!ip4addr_aton(argv[2], &peer) || !ip4_addr_get_u32(&peer) ||
	    ip4_addr_ismulticast(&peer) || ip4_addr_get_u32(&peer) == IPADDR_BROADCAST ||
	    strlen(argv[3]) != WEBTERM_TOKEN_LEN) {
		printf("Use one client IPv4 address and a fresh 32-digit random hex token\n");
		return CMD_RET_FAILURE;
	}
	for (i = 0; i < WEBTERM_TOKEN_LEN; i++)
		if (request_hex_digit(argv[3][i]) < 0)
			return CMD_RET_FAILURE;

	webterm_lock();
	webterm_peer = peer;
	memcpy(webterm_token, argv[3], sizeof(webterm_token));
	webterm_start = get_timer(0);
	webterm_unlocked = 1;
	printf("Web terminal armed for one client for ten minutes. Use an isolated direct link.\n");
	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	webterm, 4, 0, do_webterm_access,
	"control the Webfailsafe terminal from the local console",
	"allow <client-ipv4> <32-digit-hex-token>\n"
	"webterm lock"
);

static int webterm_authorized(const struct failsafe_httpd_state *hs,
			      const char *data, size_t size)
{
	const char *end = request_headers_end(data, size);
	const char *token;
	size_t i, length;
	unsigned char difference = 0;

	if (!end || !webterm_active() ||
	    !ip4_addr_cmp(&webterm_peer, ip_2_ip4(&hs->pcb->remote_ip)) ||
	    request_header(data, end, "X-Webterm-Token", &token, &length) != 1 ||
	    length != WEBTERM_TOKEN_LEN)
		return 0;
	for (i = 0; i < WEBTERM_TOKEN_LEN; i++)
		difference |= (unsigned char)(token[i] ^ webterm_token[i]);
	return difference == 0;
}

static void webterm_flush_line(void) {
	webterm_line_buffer[webterm_line_pos] = '\n';
	webterm_batch_copy(webterm_line_buffer, webterm_line_pos + 1);
	webterm_line_pos = 0;
}

static void webterm_add_char(char c) {
	if (!webterm_out.buffer)
		return;

	if (c == '\n' || c == '\r') {
		if (c == '\n' && webterm_prev_char_was_cr) {
			webterm_prev_char_was_cr = 0;
			return;
		}
		webterm_prev_char_was_cr = (c == '\r');
		if (webterm_line_pos > 0)
			webterm_flush_line();
		return;
	}

	webterm_prev_char_was_cr = 0;

	if (webterm_line_pos >= WEBTERM_LINE_SIZE - 1)
		webterm_flush_line();

	webterm_line_buffer[webterm_line_pos++] = c;
}

static void webterm_batch_copy(const char *src, int len) {
	int first = min(len, webterm_out.size - webterm_out.head);
	if (first > 0) {
		memcpy(&webterm_out.buffer[webterm_out.head], src, first);
		webterm_out.head = (webterm_out.head + first) % webterm_out.size;
	}
	if (len > first) {
		memcpy(webterm_out.buffer, src + first, len - first);
		webterm_out.head = len - first;
	}
	webterm_out.count += len;
	if (webterm_out.count > webterm_out.size) {
		webterm_out.tail = (webterm_out.tail + webterm_out.count - webterm_out.size) % webterm_out.size;
		webterm_out.count = webterm_out.size;
		webterm_out.overflow = 1;
	}
	webterm_output_seq += len;
}

void webterm_capture_output(const char *str) {
	if (!webterm_unlocked || !str || !webterm_out.buffer)
		return;
	while (*str)
		webterm_add_char(*str++);
}

void webterm_putc(const char c) {
	if (webterm_unlocked)
		webterm_add_char(c);
}

int webterm_get_output(char *buf, int size, int since) {
	int len, skip, pos, first;

	if (buf && size > 0) buf[0] = '\0';
	if (since < 0) since = 0;
	if (!buf || !webterm_out.buffer || size <= 0 || webterm_out.count <= 0 || since >= webterm_output_seq)
		return 0;

	len = min(webterm_output_seq - since, webterm_out.count);
	if (len > size - 1)
		len = size - 1;
	skip = webterm_out.count - len;
	pos = (webterm_out.tail + skip) % webterm_out.size;
	first = min(len, webterm_out.size - pos);

	memcpy(buf, &webterm_out.buffer[pos], first);
	if (len > first)
		memcpy(buf + first, webterm_out.buffer, len - first);

	buf[len] = '\0';
	return len;
}

void webterm_execute_command(const char *cmd) {
	webterm_abort_requested = 0;
	snprintf(webterm_output_buf, WEBTERM_BUFFER_SIZE, "> %s\n", cmd);
	webterm_capture_output(webterm_output_buf);
	if (webfailsafe_is_running) {
		strncpy(webterm_pending_cmd, cmd, sizeof(webterm_pending_cmd) - 1);
		webterm_pending_cmd[sizeof(webterm_pending_cmd) - 1] = '\0';
		webterm_has_pending_cmd = 1;
	} else {
		run_command(cmd, 0);
		if (webterm_line_pos > 0) webterm_flush_line();
	}
}

int webterm_run_pending_command(void) {
	if (!webterm_active() || !webterm_has_pending_cmd)
		return 0;
	webterm_has_pending_cmd = 0;
	webterm_abort_requested = 0;
	run_command(webterm_pending_cmd, 0);
	if (webterm_line_pos > 0) webterm_flush_line();
	return 1;
}

static void webterm_respond(struct failsafe_httpd_state *hs, int code, const char *ctype, const char *fmt, ...) {
	va_list ap;
	int written;
	const char *reason = code == 200 ? "OK" : code == 403 ? "Forbidden" :
			     code == 404 ? "Not Found" : code == 405 ? "Method Not Allowed" : "Bad Request";
	int hlen = snprintf(webterm_response_buf, sizeof(webterm_response_buf),
		"HTTP/1.1 %d %s\r\nContent-Type: %s\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n", code, reason, ctype);
	va_start(ap, fmt);
	written = vsnprintf(webterm_response_buf + hlen, sizeof(webterm_response_buf) - hlen, fmt, ap);
	va_end(ap);
	if (written > 0)
		hlen += min(written, (int)sizeof(webterm_response_buf) - hlen - 1);
	hs->state = STATE_FILE_REQUEST;
	hs->dataptr = (u8_t *)webterm_response_buf;
	hs->upload = hlen;
	httpd_send_data(hs);
}

static int webterm_path_is(const char *path, const char *name)
{
	size_t len = strlen(name);

	return !strncmp(path, name, len) && (path[len] == ' ' || path[len] == '?');
}

void webterm_http_handler(struct failsafe_httpd_state *hs, char *data, int data_len)
{
	const char *path, *body;
	int is_post, len, since;
	char cmd[WEBTERM_MAX_CMD_LEN];
	size_t length;

	if (!webterm_authorized(hs, data, data_len)) {
		webterm_respond(hs, 403, "text/plain", "Terminal locked\n");
		return;
	}
	is_post = !strncmp(data, "POST ", 5);
	path = data + (is_post ? 5 : 4);
	if (strncmp(path, "/webterm/", 9)) {
		webterm_respond(hs, 404, "text/plain", "Unknown terminal route\n");
		return;
	}
	path += 9;

	if (is_post && webterm_path_is(path, "cmd")) {
		body = request_body(data, data_len, &length);
		if (!body || !length || length >= sizeof(cmd) ||
		    memchr(body, '\r', length) || memchr(body, '\n', length)) {
			webterm_respond(hs, 400, "text/plain", "Invalid command body\n");
			return;
		}
		memcpy(cmd, body, length);
		cmd[length] = '\0';
		webterm_execute_command(cmd);
	} else if (is_post && webterm_path_is(path, "abort")) {
		webterm_abort_requested = 1;
	} else if (is_post && webterm_path_is(path, "reset")) {
		webterm_reset();
	} else if (!is_post && webterm_path_is(path, "status")) {
		if (webterm_line_pos > 0)
			webterm_flush_line();
		webterm_respond(hs, 200, "text/plain", "%d", webterm_output_seq);
		return;
	} else if (!is_post && webterm_path_is(path, "data")) {
		if (webterm_line_pos > 0)
			webterm_flush_line();
		since = !strncmp(path, "data?since=", 11) ? simple_strtol(path + 11, NULL, 10) : 0;
		len = webterm_get_output(webterm_output_buf, sizeof(webterm_output_buf), since);
		webterm_respond(hs, 200, "text/plain; charset=utf-8", "%s", len > 0 ? webterm_output_buf : "");
		return;
	} else {
		webterm_respond(hs, 405, "text/plain", "Unsupported terminal request\n");
		return;
	}

	webterm_respond(hs, 200, "text/plain", "OK\n");
}

void webterm_puts(const char *str) {
	webterm_capture_output(str);
	puts(str);
}