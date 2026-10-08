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

#ifndef _PLAT_COMMON_H_
#define _PLAT_COMMON_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dpaltypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef SIZEOF
#define SIZEOF(t) (sizeof (t))
#endif

#ifndef CHARSMAX
#define CHARSMAX(a) (SIZEOF (a) - 1)
#endif

#ifndef OFFSETOF
/**
 * Computes the offset of a member within the struct.
 */
#define OFFSETOF(s, m) ((DPSize)((DPChar *)&((s *)0)->m - (DPChar *)0))
#endif

#ifndef COUNTOF
/**
 * Gets the count of elements for statically sized arrays.
 */
#define COUNTOF(a)                                                            \
  ((SIZEOF (a) / SIZEOF (0 [a])) / ((DPSize)(!(SIZEOF (a) % SIZEOF (0 [a])))))
#endif

#ifndef COMPILE_ASSERT
/**
 * Validates at compile time that the predicate is true without generating
 * code. It can be used at any point in a source file where typedef is legal.
 *
 * On success, compilation proceeds normally.
 *
 * On failure, attempts to typedef an array type of negative size.
 *
 * @param expr The expression to test. It must evaluate to something that can
 * be coerced to a normal C boolean.
 */
#define COMPILE_ASSERT(expr) _DP_COMPILE_ASSERT (expr, __LINE__)

#define _DP_JOIN(a, b) a##b
#define _DP_COMPILE_ASSERT(expr, line)                                        \
  typedef char _DP_JOIN (assertion_failed_, line)[2 * !!(expr)-1]
#endif

#endif
