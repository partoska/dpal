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

#include "core/error.h"
#include "plat/common.h"
#include "plat/logger.h"
#include "plat/str.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static DPStatus gLastErrorCode = DP_EINIT;
static const DPChar *gLastErrorDetail = NULL;

#define ERROR_START (DP_EINIT)
#define ERROR_END (DP_ECONF)

static const DPChar *ERROR_NONE = "No error";
static const DPChar *ERROR_BREAK = "No error (break)";
static const DPChar *ERROR_NOT_IMPLEMENTED = "Function is not implemented";
static const DPChar *ERROR_UNEXPECTED = "Unexpected error";

static const DPChar *ERRORS[] = {
  // DP_EINIT
  "Not initialized",
  // DP_EALLOC
  "Allocation error",
  // DP_ENULL
  "Null pointer",
  // DP_EINVALID
  "Invalid value",
  // DP_ENOT_FOUND
  "Not found",
  // DP_EACCESS
  "Permission denied",
  // DP_EDOUBLE_INIT
  "Library initialized more than once",
  // DP_EPIPE
  "Pipe creation failed",
  // DP_ECLOSE
  "Close failed",
  // DP_EREAD
  "Read failed",
  // DP_EWRITE
  "Write failed",
  // DP_EWAIT
  "Wait for process failed",
  // DP_EPID
  "Invalid PID",
  // DP_EFD
  "Invalid file descriptor",
  // DP_EPROCESS_CREATE
  "Process start failed",
  // DP_EPROCESS_STOP
  "Process termination failed",
  // DP_ELOG
  "Log failed",
  // DP_ETOOLONG
  "Provided input is too long",
  // DP_ENOT_STOPPED
  "Process is not stopped",
  // DP_EBOUND
  "Out of bound access",
  // DP_EEXIST
  "Process already exists",
  // DP_ESELECT
  "Select failed",
  // DP_ECONF
  "Invalid configuration file",
};

COMPILE_ASSERT (ERROR_END - ERROR_START + 1 == COUNTOF (ERRORS));

static DPStatus
dpGetErrorStrByCode (DPStatus code, DPChar *err, DPSize sz, DPSize *used)
{
  CHECK_NULL_RETURN (err);

  const DPChar *error = NULL;
  switch (code)
    {
    case DP_OK:
      error = ERROR_NONE;
      break;

    case DP_BREAK:
      error = ERROR_BREAK;
      break;

    case DP_EINIT:
    case DP_EALLOC:
    case DP_ENULL:
    case DP_EINVALID:
    case DP_ENOT_FOUND:
    case DP_EACCESS:
    case DP_EDOUBLE_INIT:
    case DP_EPIPE:
    case DP_ECLOSE:
    case DP_EREAD:
    case DP_EWRITE:
    case DP_EWAIT:
    case DP_EPID:
    case DP_EFD:
    case DP_EPROCESS_CREATE:
    case DP_EPROCESS_STOP:
    case DP_ELOG:
    case DP_ETOOLONG:
    case DP_ENOT_STOPPED:
    case DP_EBOUND:
    case DP_EEXIST:
    case DP_ESELECT:
    case DP_ECONF:
      error = ERRORS[code - ERROR_START];
      break;

    case DP_ENOT_IMPLEMENTED:
      error = ERROR_NOT_IMPLEMENTED;
      break;

    case DP_EUNEXPECTED:
      error = ERROR_UNEXPECTED;
      break;
    }

  if (error == NULL)
    {
      error = ERROR_UNEXPECTED;
    }

  dpStrncpy (err, error, sz);
  if (used != NULL)
    {
      *used = MIN (dpStrlen (error) + 1, sz);
    }

  return DP_OK;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpSetError (DPStatus code, const DPChar *detail)
{
  if (code == DP_OK)
    {
      return DP_OK;
    }

  gLastErrorCode = code;
  gLastErrorDetail = detail;

  return gLastErrorCode;
}

DPStatus
dpPrintErrorByCode (DPStatus code, const DPChar *detail, const DPChar *prefix)
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

#if _DEBUG
  static const DPChar *INFO_EMPTY = "No additional info";
  const DPChar **info = detail != NULL ? &detail : &INFO_EMPTY;
#else
  UNUSED (detail);
  static const DPChar *INFO_EMPTY = "";
  const DPChar **info = &INFO_EMPTY;
#endif

  switch (code)
    {
    case DP_OK:
      DP_DEBUG ("%s%s%s", *pre, *delim, ERROR_NONE);
      break;
    case DP_BREAK:
      DP_DEBUG ("%s%s%s", *pre, *delim, ERROR_BREAK);
      break;

    case DP_EINIT:
    case DP_EALLOC:
    case DP_ENULL:
    case DP_EINVALID:
    case DP_ENOT_FOUND:
    case DP_EACCESS:
    case DP_EDOUBLE_INIT:
    case DP_EPIPE:
    case DP_ECLOSE:
    case DP_EREAD:
    case DP_EWRITE:
    case DP_EWAIT:
    case DP_EPID:
    case DP_EFD:
    case DP_EPROCESS_CREATE:
    case DP_EPROCESS_STOP:
    case DP_ELOG:
    case DP_ETOOLONG:
    case DP_ENOT_STOPPED:
    case DP_EBOUND:
    case DP_EEXIST:
    case DP_ESELECT:
    case DP_ECONF:
      DP_ERROR ("%s%s%s %s", *pre, *delim, ERRORS[code - ERROR_START], *info);
      break;

    case DP_ENOT_IMPLEMENTED:
      DP_ERROR ("%s%s%s %s", *pre, *delim, ERROR_NOT_IMPLEMENTED, *info);
      break;

    case DP_EUNEXPECTED:
      DP_FATAL ("%s%s%s %s", *pre, *delim, ERROR_UNEXPECTED, *info);
      break;
    }

  return DP_OK;
}

DPStatus
dpPrintError (const DPChar *prefix)
{
  return dpPrintErrorByCode (gLastErrorCode, gLastErrorDetail, prefix);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpGetErrorStr (DPChar *err, DPSize sz, DPSize *used)
{
  return dpGetErrorStrByCode (gLastErrorCode, err, sz, used);
}
