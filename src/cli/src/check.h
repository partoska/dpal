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

#ifndef _CHECK_H_
#define _CHECK_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dpaltypes.h"

#include "plat.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define CHECK_DPAL(status, name, cleanup)                                     \
  do                                                                          \
    {                                                                         \
      if ((status) != DP_OK)                                                  \
        {                                                                     \
          dpcPrintDpalError (name);                                           \
          cleanup                                                             \
        }                                                                     \
    }                                                                         \
  while (FALSE)

#define CHECK_POSIX(retval, name, cleanup)                                    \
  do                                                                          \
    {                                                                         \
      if ((retval) < 0)                                                       \
        {                                                                     \
          DPC_ERROR (name);                                                   \
          cleanup                                                             \
        }                                                                     \
    }                                                                         \
  while (FALSE)

#define CHECK_GENERIC(expr, name, cleanup)                                    \
  do                                                                          \
    {                                                                         \
      if (!(expr))                                                            \
        {                                                                     \
          DPC_ERROR (name);                                                   \
          cleanup                                                             \
        }                                                                     \
    }                                                                         \
  while (FALSE)

#define EXIT_WITH_FAILURE                                                     \
  {                                                                           \
    dpcExit (1);                                                              \
  }
#define EXIT_WITH_SUCCESS                                                     \
  {                                                                           \
    dpcExit (0);                                                              \
  }
#define RETURN_WITH_FAILURE                                                   \
  {                                                                           \
    return DPC_EFAIL;                                                         \
  }
#define RETURN_WITH_SUCCESS                                                   \
  {                                                                           \
    return DPC_OK;                                                            \
  }

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef enum DPCEStatus
{
  DPC_OK = 0x00,
  DPC_EFAIL = -0x01,
  DPC_ENOT_IMPLEMENTED = -0xFE,
  DPC_EUNEXPECTED = -0xFF,
} DPCStatus;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPCStatus dpcPrintDpalError (const DPChar *prefix);
DPCStatus dpcGetDpalErrorStr (DPChar *err, DPSize sz, DPSize *used);

#endif
