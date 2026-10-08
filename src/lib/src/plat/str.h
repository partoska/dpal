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

#ifndef _PLAT_STR_H_
#define _PLAT_STR_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dpaltypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API void *dpMemset (void *dst, DPInt val, DPSize n);
DP_API void *dpMemcpy (void *restrict dst, const void *restrict src, DPSize n);

DP_API DPSize dpStrlen (const DPChar *src);
DP_API DPSize dpStrnlen (const DPChar *src, DPSize max);
DP_API DPInt dpStrcmp (const DPChar *src1, const DPChar *src2);
DP_API DPInt dpStrncmp (const DPChar *src1, const DPChar *src2, DPSize n);
DP_API DPChar *dpStrncpy (DPChar *dst, const DPChar *src, DPSize n);
DP_API DPChar *dpStrndup (const DPChar *src, DPSize n);
DP_API DPChar *dpStrncat (DPChar *dst, const DPChar *src, DPSize n);
DP_API DPQword dpStrtoul (const DPChar *restrict nptr,
                          DPChar **restrict endptr, DPInt base);
DP_API DPLong dpStrtol (const DPChar *restrict nptr, DPChar **restrict endptr,
                        DPInt base);

DP_API DPChar *dpItoa10 (DPInt num, DPChar *str);
DP_API DPChar *dpBasename (DPChar *path);

#endif
