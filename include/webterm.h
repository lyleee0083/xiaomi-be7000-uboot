/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef BE7000_WEBTERM_H
#define BE7000_WEBTERM_H
#include <linux/types.h>

#define WEBTERM_COMMAND_MAX 4096
#define WEBTERM_OUTPUT_MAX (12 * 1024)

int webterm_enqueue(const char *command, size_t length);
void webterm_capture(const char *data, size_t length);
size_t webterm_read(char *output, size_t length);
int webterm_busy(void);
int webterm_running(void);
int webterm_input_pending(void);
int webterm_send_input(const char *data, size_t length);
int webterm_get_input(void);
void webterm_abort(void);
int webterm_abort_requested(void);
void webterm_poll(void);
void webterm_reset(void);
#endif
