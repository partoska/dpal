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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdarg.h>
#include <stdio.h>

#include "plat/common.h"
#include "plat/io.h"
#include "plat/logger.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPInt
dpPrint (DPFd fd, const DPChar *fmt, ...)
{
  va_list args;
  va_start (args, fmt);

  int result = vdprintf (fd, fmt, args);

  va_end (args);

  return result;
}

DPInt
dpInfo (const DPChar *fmt, ...)
{
  va_list args;
  va_start (args, fmt);

  int result = vdprintf (DP_FD_STDOUT, fmt, args);

  va_end (args);

  return result;
}

DPInt
dpWarn (const DPChar *fmt, ...)
{
  va_list args;
  va_start (args, fmt);

  int result = vdprintf (DP_FD_STDERR, fmt, args);

  va_end (args);

  return result;
}

DPInt
dpError (const DPChar *fmt, ...)
{
  va_list args;
  va_start (args, fmt);

  int result = vdprintf (DP_FD_STDERR, fmt, args);

  va_end (args);

  return result;
}

DPInt
dpFatal (const DPChar *fmt, ...)
{
  va_list args;
  va_start (args, fmt);

  int result = vdprintf (DP_FD_STDERR, fmt, args);

  va_end (args);

  return result;
}

DPInt
dpDebug (const DPChar *fmt, ...)
{
#ifdef _DEBUG
  va_list args;
  va_start (args, fmt);

  int result = vdprintf (DP_FD_STDOUT, fmt, args);

  va_end (args);

  return result;
#else
  UNUSED (fmt);
  return 0;
#endif
}

