/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef BE7000_TLS_LIMITS_H
#define BE7000_TLS_LIMITS_H
#ifndef CHAR_BIT
#define CHAR_BIT __CHAR_BIT__
#endif
#ifndef INT_MAX
#define INT_MAX __INT_MAX__
#endif
#ifndef INT_MIN
#define INT_MIN (-INT_MAX - 1)
#endif
#ifndef UINT_MAX
#define UINT_MAX (INT_MAX * 2U + 1U)
#endif
#ifndef LONG_MAX
#define LONG_MAX __LONG_MAX__
#endif
#ifndef LONG_MIN
#define LONG_MIN (-LONG_MAX - 1L)
#endif
#ifndef ULONG_MAX
#define ULONG_MAX (LONG_MAX * 2UL + 1UL)
#endif
#ifndef UCHAR_MAX
#define UCHAR_MAX (__SCHAR_MAX__ * 2U + 1U)
#endif
#endif
