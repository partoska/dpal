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

#ifndef _LOGGER_H_
#define _LOGGER_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dpaltypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Generic prefix. */
#define _DPCL ""

/* Info prefix. */
#define _DPCLI ""

/* Warning prefix. */
#define _DPCLW "warning: "

/* Error prefix. */
#define _DPCLE "error: "

/* Fatal error prefix. */
#define _DPCLF "fatal: "

/* Debug message prefix. */
#define _DPCLD "debug: "

/* Verbose debug message prefix. */
#define _DPCLV "verbose: "

/* New line. */
#define _DPCLNL "\n"

/* Info message mapping. */
#define _DPC_INFO_F(...) dpcInfo (__VA_ARGS__)

/* Warning message mapping. */
#define _DPC_WARN_F(...) dpcWarn (__VA_ARGS__)

/* Error message mapping. */
#define _DPC_ERROR_F(...) dpcError (__VA_ARGS__)

/* Fatal error message mapping. */
#define _DPC_FATAL_F(...) dpcFatal (__VA_ARGS__)

#ifdef _DEBUG
/* Debug message mapping. */
#define _DPC_DEBUG_F(...) dpcDebug (__VA_ARGS__)
#else
/* Debug message mapping. */
#define _DPC_DEBUG_F(...)
#endif

#if DEBUG_SLOW
/* Verbose debug message mapping. */
#define _DPC_DSLOW_F(...) dpcDebug (__VA_ARGS__)
#else
/* Verbose debug message mapping. */
#define _DPC_DSLOW_F(...)
#endif

#define DPC_INFO(...) _DPC_VFUNC (_DPC_INFO, __VA_ARGS__)
#define DPC_WARN(...) _DPC_VFUNC (_DPC_WARN, __VA_ARGS__)
#define DPC_ERROR(...) _DPC_VFUNC (_DPC_ERROR, __VA_ARGS__)
#define DPC_FATAL(...) _DPC_VFUNC (_DPC_FATAL, __VA_ARGS__)
#define DPC_DEBUG(...) _DPC_VFUNC (_DPC_DEBUG, __VA_ARGS__)
#define DPC_DSLOW(...) _DPC_VFUNC (_DPC_DSLOW, __VA_ARGS__)

/**
 * Calculates the number of arguments.
 */
#define _DPC_NARG(...) _DPC_NARG_I (__VA_ARGS__, _DPC_RSEQ_N ())
#define _DPC_NARG_I(...) _DPC_ARG_N (__VA_ARGS__)
#define _DPC_ARG_N(_1, _2, _3, _4, _5, _6, N, ...) N
#define _DPC_RSEQ_N() 6, 5, 4, 3, 2, 1, 0

/**
 * Converts func to funcX where "X" denotes the argument count.
 */
#define _DPC_VFUNC_INNER(name, n) name##n
#define _DPC_VFUNC_OUTER(name, n) _DPC_VFUNC_INNER (name, n)
#define _DPC_VFUNC(func, ...)                                                 \
  _DPC_VFUNC_OUTER (func, _DPC_NARG (__VA_ARGS__)) (__VA_ARGS__)

#define _DPC_INFO1(fmt) _DPC_INFO_F (_DPCL _DPCLI fmt _DPCLNL)
#define _DPC_INFO2(fmt, a1) _DPC_INFO_F (_DPCL _DPCLI fmt _DPCLNL, a1)
#define _DPC_INFO3(fmt, a1, a2) _DPC_INFO_F (_DPCL _DPCLI fmt _DPCLNL, a1, a2)
#define _DPC_INFO4(fmt, a1, a2, a3)                                           \
  _DPC_INFO_F (_DPCL _DPCLI fmt _DPCLNL, a1, a2, a3)
#define _DPC_INFO5(fmt, a1, a2, a3, a4)                                       \
  _DPC_INFO_F (_DPCL _DPCLI fmt _DPCLNL, a1, a2, a3, a4)
#define _DPC_INFO6(fmt, a1, a2, a3, a4, a5)                                   \
  _DPC_INFO_F (_DPCL _DPCLI fmt _DPCLNL, a1, a2, a3, a4, a5)

#define _DPC_WARN1(fmt) _DPC_WARN_F (_DPCL _DPCLW fmt _DPCLNL)
#define _DPC_WARN2(fmt, a1) _DPC_WARN_F (_DPCL _DPCLW fmt _DPCLNL, a1)
#define _DPC_WARN3(fmt, a1, a2) _DPC_WARN_F (_DPCL _DPCLW fmt _DPCLNL, a1, a2)
#define _DPC_WARN4(fmt, a1, a2, a3)                                           \
  _DPC_WARN_F (_DPCL _DPCLW fmt _DPCLNL, a1, a2, a3)
#define _DPC_WARN5(fmt, a1, a2, a3, a4)                                       \
  _DPC_WARN_F (_DPCL _DPCLW fmt _DPCLNL, a1, a2, a3, a4)
#define _DPC_WARN6(fmt, a1, a2, a3, a4, a5)                                   \
  _DPC_WARN_F (_DPCL _DPCLW fmt _DPCLNL, a1, a2, a3, a4, a5)

#define _DPC_ERROR1(fmt) _DPC_ERROR_F (_DPCL _DPCLE fmt _DPCLNL)
#define _DPC_ERROR2(fmt, a1) _DPC_ERROR_F (_DPCL _DPCLE fmt _DPCLNL, a1)
#define _DPC_ERROR3(fmt, a1, a2)                                              \
  _DPC_ERROR_F (_DPCL _DPCLE fmt _DPCLNL, a1, a2)
#define _DPC_ERROR4(fmt, a1, a2, a3)                                          \
  _DPC_ERROR_F (_DPCL _DPCLE fmt _DPCLNL, a1, a2, a3)
#define _DPC_ERROR5(fmt, a1, a2, a3, a4)                                      \
  _DPC_ERROR_F (_DPCL _DPCLE fmt _DPCLNL, a1, a2, a3, a4)
#define _DPC_ERROR6(fmt, a1, a2, a3, a4, a5)                                  \
  _DPC_ERROR_F (_DPCL _DPCLE fmt _DPCLNL, a1, a2, a3, a4, a5)

#define _DPC_FATAL1(fmt) _DPC_FATAL_F (_DPCL _DPCLF fmt _DPCLNL)
#define _DPC_FATAL2(fmt, a1) _DPC_FATAL_F (_DPCL _DPCLF fmt _DPCLNL, a1)
#define _DPC_FATAL3(fmt, a1, a2)                                              \
  _DPC_FATAL_F (_DPCL _DPCLF fmt _DPCLNL, a1, a2)
#define _DPC_FATAL4(fmt, a1, a2, a3)                                          \
  _DPC_FATAL_F (_DPCL _DPCLF fmt _DPCLNL, a1, a2, a3)
#define _DPC_FATAL5(fmt, a1, a2, a3, a4)                                      \
  _DPC_FATAL_F (_DPCL _DPCLF fmt _DPCLNL, a1, a2, a3, a4)
#define _DPC_FATAL6(fmt, a1, a2, a3, a4, a5)                                  \
  _DPC_FATAL_F (_DPCL _DPCLF fmt _DPCLNL, a1, a2, a3, a4, a5)

#define _DPC_DEBUG1(fmt) _DPC_DEBUG_F (_DPCL _DPCLD fmt _DPCLNL)
#define _DPC_DEBUG2(fmt, a1) _DPC_DEBUG_F (_DPCL _DPCLD fmt _DPCLNL, a1)
#define _DPC_DEBUG3(fmt, a1, a2)                                              \
  _DPC_DEBUG_F (_DPCL _DPCLD fmt _DPCLNL, a1, a2)
#define _DPC_DEBUG4(fmt, a1, a2, a3)                                          \
  _DPC_DEBUG_F (_DPCL _DPCLD fmt _DPCLNL, a1, a2, a3)
#define _DPC_DEBUG5(fmt, a1, a2, a3, a4)                                      \
  _DPC_DEBUG_F (_DPCL _DPCLD fmt _DPCLNL, a1, a2, a3, a4)
#define _DPC_DEBUG6(fmt, a1, a2, a3, a4, a5)                                  \
  _DPC_DEBUG_F (_DPCL _DPCLD fmt _DPCLNL, a1, a2, a3, a4, a5)

#define _DPC_DSLOW1(fmt) _DPC_DSLOW_F (_DPCL _DPCLV fmt _DPCLNL)
#define _DPC_DSLOW2(fmt, a1) _DPC_DSLOW_F (_DPCL _DPCLV fmt _DPCLNL, a1)
#define _DPC_DSLOW3(fmt, a1, a2)                                              \
  _DPC_DSLOW_F (_DPCL _DPCLV fmt _DPCLNL, a1, a2)
#define _DPC_DSLOW4(fmt, a1, a2, a3)                                          \
  _DPC_DSLOW_F (_DPCL _DPCLV fmt _DPCLNL, a1, a2, a3)
#define _DPC_DSLOW5(fmt, a1, a2, a3, a4)                                      \
  _DPC_DSLOW_F (_DPCL _DPCLV fmt _DPCLNL, a1, a2, a3, a4)
#define _DPC_DSLOW6(fmt, a1, a2, a3, a4, a5)                                  \
  _DPC_DSLOW_F (_DPCL _DPCLV fmt _DPCLNL, a1, a2, a3, a4, a5)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API DPInt dpcInfo (const DPChar *fmt, ...);
DP_API DPInt dpcWarn (const DPChar *fmt, ...);
DP_API DPInt dpcError (const DPChar *fmt, ...);
DP_API DPInt dpcFatal (const DPChar *fmt, ...);
DP_API DPInt dpcDebug (const DPChar *fmt, ...);

#endif
