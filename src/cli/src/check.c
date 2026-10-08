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

#include "dpal.h"

#include "check.h"
#include "logger.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPCStatus
dpcPrintDpalError (const DPChar *prefix)
{
  static const DPChar *PREFIX_EMPTY = "";
  static const DPChar *DELIM_EMPTY = "";
  static const DPChar *DELIM_COLON = ": ";

  const DPChar **pre;
  const DPChar **delim;
  if (prefix != NULL)
    {
      pre = &prefix;
      delim = &DELIM_COLON;
    }
  else
    {
      pre = &PREFIX_EMPTY;
      delim = &DELIM_EMPTY;
    }

  DPChar buff[64] = { 0 };
  CHECK_DPAL (dpGetErrorStr (buff, CHARSMAX (buff), NULL),
              "Cannot print error cause", RETURN_WITH_FAILURE);
  DPC_ERROR ("%s%s%s", *pre, *delim, buff);

  return DPC_OK;
}

DPCStatus
dpcGetDpalErrorStr (DPChar *err, DPSize sz, DPSize *used)
{
  CHECK_DPAL (dpGetErrorStr (err, sz, used), "Cannot get error cause",
              RETURN_WITH_FAILURE);

  return DPC_OK;
}
