/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include <webterm.h>

static char pending[WEBTERM_COMMAND_MAX + 1];
static char output[WEBTERM_OUTPUT_MAX];
static unsigned int start, stored;
static bool running, abort_requested;
static char input[WEBTERM_COMMAND_MAX];
static size_t input_pos, input_len;

int webterm_enqueue(const char *command, size_t length)
{
	if (!length || length > WEBTERM_COMMAND_MAX ||
	    memchr(command, 0, length))
		return -EINVAL;
	if (running || pending[0])
		return -EBUSY;
	memcpy(pending, command, length);
	pending[length] = 0;
	start = stored = 0;
	return 0;
}

void webterm_capture(const char *data, size_t length)
{
	unsigned int next;

	if (!running)
		return;
	for (; length; --length, ++data) {
		next = (start + stored) % sizeof(output);
		output[next] = *data;
		if (stored == sizeof(output))
			start = (start + 1) % sizeof(output);
		else
			stored++;
	}
}

size_t webterm_read(char *destination, size_t length)
{
	size_t copied = min(length, (size_t)stored);
	size_t first = min(copied, sizeof(output) - start);

	memcpy(destination, output + start, first);
	memcpy(destination + first, output, copied - first);
	return copied;
}

int webterm_busy(void)
{
	return running || pending[0];
}

int webterm_running(void)
{
	return running;
}

int webterm_input_pending(void)
{
	return running && (abort_requested || input_pos < input_len);
}

int webterm_send_input(const char *data, size_t length)
{
	if (!running || input_pos < input_len)
		return -EBUSY;
	if (!length || length > sizeof(input))
		return -EINVAL;
	memcpy(input, data, length);
	input_pos = 0;
	input_len = length;
	return 0;
}

int webterm_get_input(void)
{
	if (webterm_abort_requested())
		return 3;	/* Control-C */
	return (unsigned char)input[input_pos++];
}

void webterm_abort(void)
{
	if (running) {
		abort_requested = true;
	} else if (pending[0]) {
		static const char cancelled[] = "\n[exit -4]\n";

		pending[0] = 0;
		memcpy(output, cancelled, sizeof(cancelled) - 1);
		start = 0;
		stored = sizeof(cancelled) - 1;
	}
}

int webterm_abort_requested(void)
{
	if (!abort_requested)
		return 0;
	abort_requested = false;
	return 1;
}

void webterm_poll(void)
{
	int status;
	char result[48];

	if (running || !pending[0])
		return;
	running = true;
	abort_requested = false;
	input_pos = input_len = 0;
	status = run_command(pending, 0);
	snprintf(result, sizeof(result), "\n[exit %d]\n", status);
	webterm_capture(result, strlen(result));
	pending[0] = 0;
	running = false;
}

void webterm_reset(void)
{
	start = stored = 0;
	input_pos = input_len = 0;
	if (running)
		abort_requested = true;
	else
		pending[0] = 0;
}
