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

#include "dpaltypes.h"

#include "check.h"
#include "list.h"
#include "logger.h"
#include "plat.h"
#include "util.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define JSON_ESC_MAX (CMD_NAME_MAX * 6 + 1)
#define CSV_QUOTE_MAX (CMD_NAME_MAX * 2 + 3)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void
dpcJsonEsc (DPChar *dst, const DPChar *src)
{
  static const DPChar hex[] = "0123456789abcdef";

  DPSize j = 0;
  for (DPSize i = 0; i < CMD_NAME_MAX && src[i] != '\0'; ++i)
    {
      const DPByte c = (DPByte)src[i];
      if (c == '"' || c == '\\')
        {
          dst[j++] = '\\';
          dst[j++] = (DPChar)c;
        }
      else if (c < 0x20)
        {
          dst[j++] = '\\';
          dst[j++] = 'u';
          dst[j++] = '0';
          dst[j++] = '0';
          dst[j++] = hex[c >> 4];
          dst[j++] = hex[c & 0x0F];
        }
      else
        {
          dst[j++] = (DPChar)c;
        }
    }
  dst[j] = '\0';
}

static void
dpcCsvQuote (DPChar *dst, const DPChar *src)
{
  DPSize j = 0;
  dst[j++] = '"';
  for (DPSize i = 0; i < CMD_NAME_MAX && src[i] != '\0'; ++i)
    {
      if (src[i] == '"')
        {
          dst[j++] = '"';
        }
      dst[j++] = src[i];
    }
  dst[j++] = '"';
  dst[j] = '\0';
}

// Prints a JSON array element, followed by a comma unless it is the last.
static void
dpcJsonRow (const DPCMsgProcInfo *info, DPBool more)
{
  DPChar name[JSON_ESC_MAX];
  dpcJsonEsc (name, info->name);

  dpcInfo ("  {\"id\":%d,\"name\":\"%s\",\"state\":\"%s\",\"pid\":%d,"
           "\"restarts\":%u}%s\n",
           info->id, name, info->state == DP_STOPPED ? "stopped" : "running",
           info->pid, info->restarts, more ? "," : "");
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPInt
dpcListParseFmt (DPCFmt *fmt, const DPChar *value)
{
  if (dpcStrcmp (value, "plain") == 0)
    {
      *fmt = DPC_FPLAIN;
      return DPC_OK;
    }

  if (dpcStrcmp (value, "json") == 0)
    {
      *fmt = DPC_FJSON;
      return DPC_OK;
    }

  if (dpcStrcmp (value, "csv") == 0)
    {
      *fmt = DPC_FCSV;
      return DPC_OK;
    }

  if (dpcStrcmp (value, "one") == 0)
    {
      *fmt = DPC_FONE;
      return DPC_OK;
    }

  return DPC_EFAIL;
}

void
dpcListHeader (const DPCList *list)
{
  switch (list->fmt)
    {
    case DPC_FJSON:
      DPC_INFO ("[");
      break;

    case DPC_FCSV:
      DPC_INFO ("id,name,state,pid,restarts");
      break;

    default:
      break;
    }
}

DPInt
dpcListRow (const DPCMsgProcInfo *info, void *ctx)
{
  DPCList *list = (DPCList *)ctx;
  const DPBool stopped = info->state == DP_STOPPED;

  switch (list->fmt)
    {
    case DPC_FJSON:
      // Only the next row tells whether a row needs a trailing comma, so each
      // one is printed once the next one (or the footer) arrives. Every line
      // is written whole, so other output cannot end up in the middle of it.
      if (list->count > 0)
        {
          dpcJsonRow (&list->prev, TRUE);
        }
      list->prev = *info;
      break;

    case DPC_FCSV:
      {
        DPChar name[CSV_QUOTE_MAX];
        dpcCsvQuote (name, info->name);

        DPC_INFO ("%d,%s,%s,%d,%u", info->id, name,
                  stopped ? "stopped" : "running", info->pid, info->restarts);
        break;
      }

    case DPC_FONE:
      DPC_INFO ("%.*s", CMD_NAME_MAX, info->name);
      break;

    default:
      DPC_INFO ("%4d: %-16s %-8s [ %6d ] %8u ↻", info->id, info->name,
                stopped ? STATE_STOPPED_STR : STATE_RUNNING_STR, info->pid,
                info->restarts);
      break;
    }

  ++list->count;

  return DPC_OK;
}

void
dpcListFooter (const DPCList *list)
{
  switch (list->fmt)
    {
    case DPC_FJSON:
      // The last row is still pending (see dpcListRow).
      if (list->count > 0)
        {
          dpcJsonRow (&list->prev, FALSE);
        }
      DPC_INFO ("]");
      break;

    default:
      break;
    }
}
