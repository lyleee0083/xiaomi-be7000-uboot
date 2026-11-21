/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef BE7000_TLS_ASSERT_H
#define BE7000_TLS_ASSERT_H
void hang(void) __attribute__((noreturn));
#define assert(condition) ((condition) ? (void)0 : hang())
#endif
