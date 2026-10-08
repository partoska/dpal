/**
 * Daemon Pal - Compact user-space tool/library for process management.
 * Copyright (C) 2024 Fabrika Charvat s.r.o. All rights reserved.
 * Developed by Partoska Laboratory team, <https://lab.partoska.com>
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * You can contact the author(s) via email at ask <at> partoska.com.
 */

#ifndef _PLAT_LOGGER_H_
#define _PLAT_LOGGER_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dpaltypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Generic log prefix. */
#define _DPL "[dp] "

/* Info prefix. */
#define _DPLI ""

/* Warning prefix. */
#define _DPLW "warning: "

/* Error prefix. */
#define _DPLE "error: "

/* Fatal error prefix. */
#define _DPLF "fatal: "

/* Debug message prefix. */
#define _DPLD "debug: "

/* Verbose debug message prefix. */
#define _DPLV "verbose: "

/* New line. */
#define _DPLNL "\n"

/* Info log message mapping. */
#define _DP_INFO_F(...) dpInfo (__VA_ARGS__)

/* Warning log message mapping. */
#define _DP_WARN_F(...) dpWarn (__VA_ARGS__)

/* Error log message mapping. */
#define _DP_ERROR_F(...) dpError (__VA_ARGS__)

/* Fatal error log message mapping. */
#define _DP_FATAL_F(...) dpFatal (__VA_ARGS__)

#ifdef _DEBUG
/* Debug log message mapping. */
#define _DP_DEBUG_F(...) dpDebug (__VA_ARGS__)
#else
/* Debug log message mapping. */
#define _DP_DEBUG_F(...)
#endif

#if DEBUG_SLOW
/* Verbose debug log message mapping. */
#define _DP_DSLOW_F(...) dpDebug (__VA_ARGS__)
#else
/* Verbose debug log message mapping. */
#define _DP_DSLOW_F(...)
#endif

/**
 * Calculates the number of arguments.
 */
#define _DP_NARG(...) _DP_NARG_I (__VA_ARGS__, _DP_RSEQ_N ())
#define _DP_NARG_I(...) _DP_ARG_N (__VA_ARGS__)
#define _DP_ARG_N(_1, _2, _3, _4, _5, _6, N, ...) N
#define _DP_RSEQ_N() 6, 5, 4, 3, 2, 1, 0

/**
 * Converts func to funcX where "X" denotes the argument count.
 */
#define _DP_VFUNC_INNER(name, n) name##n
#define _DP_VFUNC_OUTER(name, n) _DP_VFUNC_INNER (name, n)
#define _DP_VFUNC(func, ...)                                                  \
  _DP_VFUNC_OUTER (func, _DP_NARG (__VA_ARGS__)) (__VA_ARGS__)

#define _DP_INFO1(fmt) _DP_INFO_F (_DPL _DPLI fmt _DPLNL)
#define _DP_INFO2(fmt, a1) _DP_INFO_F (_DPL _DPLI fmt _DPLNL, a1)
#define _DP_INFO3(fmt, a1, a2) _DP_INFO_F (_DPL _DPLI fmt _DPLNL, a1, a2)
#define _DP_INFO4(fmt, a1, a2, a3) _DP_INFO_F (_DPL _DPLI fmt _DPLNL, a1, a2, a3)
#define _DP_INFO5(fmt, a1, a2, a3, a4)                                            \
  _DP_INFO_F (_DPL _DPLI fmt _DPLNL, a1, a2, a3, a4)
#define _DP_INFO6(fmt, a1, a2, a3, a4, a5)                                        \
  _DP_INFO_F (_DPL _DPLI fmt _DPLNL, a1, a2, a3, a4, a5)

#define _DP_WARN1(fmt) _DP_WARN_F (_DPL _DPLW fmt _DPLNL)
#define _DP_WARN2(fmt, a1) _DP_WARN_F (_DPL _DPLW fmt _DPLNL, a1)
#define _DP_WARN3(fmt, a1, a2) _DP_WARN_F (_DPL _DPLW fmt _DPLNL, a1, a2)
#define _DP_WARN4(fmt, a1, a2, a3) _DP_WARN_F (_DPL _DPLW fmt _DPLNL, a1, a2, a3)
#define _DP_WARN5(fmt, a1, a2, a3, a4)                                            \
  _DP_WARN_F (_DPL _DPLW fmt _DPLNL, a1, a2, a3, a4)
#define _DP_WARN6(fmt, a1, a2, a3, a4, a5)                                        \
  _DP_WARN_F (_DPL _DPLW fmt _DPLNL, a1, a2, a3, a4, a5)

#define _DP_ERROR1(fmt) _DP_ERROR_F (_DPL _DPLE fmt _DPLNL)
#define _DP_ERROR2(fmt, a1) _DP_ERROR_F (_DPL _DPLE fmt _DPLNL, a1)
#define _DP_ERROR3(fmt, a1, a2) _DP_ERROR_F (_DPL _DPLE fmt _DPLNL, a1, a2)
#define _DP_ERROR4(fmt, a1, a2, a3) _DP_ERROR_F (_DPL _DPLE fmt _DPLNL, a1, a2, a3)
#define _DP_ERROR5(fmt, a1, a2, a3, a4)                                           \
  _DP_ERROR_F (_DPL _DPLE fmt _DPLNL, a1, a2, a3, a4)
#define _DP_ERROR6(fmt, a1, a2, a3, a4, a5)                                       \
  _DP_ERROR_F (_DPL _DPLE fmt _DPLNL, a1, a2, a3, a4, a5)

#define _DP_FATAL1(fmt) _DP_FATAL_F (_DPL _DPLF fmt _DPLNL)
#define _DP_FATAL2(fmt, a1) _DP_FATAL_F (_DPL _DPLF fmt _DPLNL, a1)
#define _DP_FATAL3(fmt, a1, a2) _DP_FATAL_F (_DPL _DPLF fmt _DPLNL, a1, a2)
#define _DP_FATAL4(fmt, a1, a2, a3) _DP_FATAL_F (_DPL _DPLF fmt _DPLNL, a1, a2, a3)
#define _DP_FATAL5(fmt, a1, a2, a3, a4)                                           \
  _DP_FATAL_F (_DPL _DPLF fmt _DPLNL, a1, a2, a3, a4)
#define _DP_FATAL6(fmt, a1, a2, a3, a4, a5)                                       \
  _DP_FATAL_F (_DPL _DPLF fmt _DPLNL, a1, a2, a3, a4, a5)

#define _DP_DEBUG1(fmt) _DP_DEBUG_F (_DPL _DPLD fmt _DPLNL)
#define _DP_DEBUG2(fmt, a1) _DP_DEBUG_F (_DPL _DPLD fmt _DPLNL, a1)
#define _DP_DEBUG3(fmt, a1, a2) _DP_DEBUG_F (_DPL _DPLD fmt _DPLNL, a1, a2)
#define _DP_DEBUG4(fmt, a1, a2, a3) _DP_DEBUG_F (_DPL _DPLD fmt _DPLNL, a1, a2, a3)
#define _DP_DEBUG5(fmt, a1, a2, a3, a4)                                           \
  _DP_DEBUG_F (_DPL _DPLD fmt _DPLNL, a1, a2, a3, a4)
#define _DP_DEBUG6(fmt, a1, a2, a3, a4, a5)                                       \
  _DP_DEBUG_F (_DPL _DPLD fmt _DPLNL, a1, a2, a3, a4, a5)

#define _DP_DSLOW1(fmt) _DP_DSLOW_F (_DPL _DPLV fmt _DPLNL)
#define _DP_DSLOW2(fmt, a1) _DP_DSLOW_F (_DPL _DPLV fmt _DPLNL, a1)
#define _DP_DSLOW3(fmt, a1, a2) _DP_DSLOW_F (_DPL _DPLV fmt _DPLNL, a1, a2)
#define _DP_DSLOW4(fmt, a1, a2, a3) _DP_DSLOW_F (_DPL _DPLV fmt _DPLNL, a1, a2, a3)
#define _DP_DSLOW5(fmt, a1, a2, a3, a4)                                           \
  _DP_DSLOW_F (_DPL _DPLV fmt _DPLNL, a1, a2, a3, a4)
#define _DP_DSLOW6(fmt, a1, a2, a3, a4, a5)                                       \
  _DP_DSLOW_F (_DPL _DPLV fmt _DPLNL, a1, a2, a3, a4, a5)

#define DP_INFO(...) _DP_VFUNC (_DP_INFO, __VA_ARGS__)
#define DP_WARN(...) _DP_VFUNC (_DP_WARN, __VA_ARGS__)
#define DP_ERROR(...) _DP_VFUNC (_DP_ERROR, __VA_ARGS__)
#define DP_FATAL(...) _DP_VFUNC (_DP_FATAL, __VA_ARGS__)
#define DP_DEBUG(...) _DP_VFUNC (_DP_DEBUG, __VA_ARGS__)
#define DP_DSLOW(...) _DP_VFUNC (_DP_DSLOW, __VA_ARGS__)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API DPInt dpPrint (DPFd fd, const DPChar *fmt, ...);
DP_API DPInt dpInfo (const DPChar *fmt, ...);
DP_API DPInt dpWarn (const DPChar *fmt, ...);
DP_API DPInt dpError (const DPChar *fmt, ...);
DP_API DPInt dpFatal (const DPChar *fmt, ...);
DP_API DPInt dpDebug (const DPChar *fmt, ...);

#endif

