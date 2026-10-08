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

#ifndef _CORE_ERROR_H_
#define _CORE_ERROR_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "plat/common.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define SET_ERROR_RETURN(error, detail)                                       \
  do                                                                          \
    {                                                                         \
      dpSetError ((error), (detail));                                         \
      return error;                                                           \
    }                                                                         \
  while (FALSE)

#define CHECK_REPORT(expr, error)                                             \
  do                                                                          \
    {                                                                         \
      if (!(expr))                                                            \
        {                                                                     \
          dpPrintErrorByCode ((error), "Failed condition: " #expr, NULL);     \
        }                                                                     \
    }                                                                         \
  while (FALSE)

#define CHECK_RETURN(expr, error)                                             \
  do                                                                          \
    {                                                                         \
      if (!(expr))                                                            \
        {                                                                     \
          DPStatus _status = (error);                                         \
          dpSetError (_status, "Failed condition: " #expr);                   \
          return _status;                                                     \
        }                                                                     \
    }                                                                         \
  while (FALSE)

#define CHECK_STATUS_RETURN(status)                                           \
  do                                                                          \
    {                                                                         \
      DPStatus _status = (status);                                            \
      if (_status != DP_OK)                                                   \
        {                                                                     \
          return _status;                                                     \
        }                                                                     \
    }                                                                         \
  while (FALSE)

#define CHECK_STATUS_REPORT(status)                                           \
  do                                                                          \
    {                                                                         \
      DPStatus _status = (status);                                            \
      if (_status != DP_OK)                                                   \
        {                                                                     \
          dpPrintError (NULL);                                                \
        }                                                                     \
    }                                                                         \
  while (FALSE)

#define CHECK_NULL_RETURN(expr) CHECK_RETURN ((expr) != NULL, DP_ENULL)
#define CHECK_ALLOC_RETURN(ptr) CHECK_RETURN ((ptr) != NULL, DP_EALLOC)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API DPStatus dpGetErrorStr (DPChar *err, DPSize sz, DPSize *used);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API DPStatus dpPrintError (const DPChar *prefix);
DP_API DPStatus dpSetError (DPStatus error, const DPChar *detail);
DP_API DPStatus dpPrintErrorByCode (DPStatus error, const DPChar *detail,
                                    const DPChar *prefix);

#endif
