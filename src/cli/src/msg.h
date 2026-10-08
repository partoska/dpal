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

#ifndef _MSG_H_
#define _MSG_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dpaltypes.h"

#include "plat/common.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define CMD_NAME_MAX (64)
#define CMD_ARG_MAX (1024)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Asserts
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

COMPILE_ASSERT (CMD_NAME_MAX <= DP_NAME_MAX);
COMPILE_ASSERT (CMD_ARG_MAX <= DP_APP_MAX);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef enum DPCEDir
{
  DPC_DC2S = 'A',
  DPC_DS2C = 'B'
} DPCDir;

typedef enum DPCECmd
{
  // Do NOT ever use this one!
  DPC_CINVALID = 0x00,

  DPC_CNONE = 0x01,
  DPC_CERROR,
  DPC_CFINISH,

  DPC_CSTART,
  DPC_CSTOP,
  DPC_CRESTART,

  DPC_CPERSIST,
  DPC_CIMPORT,
  DPC_CEXPORT,
  DPC_CCREATE,
  DPC_CDELETE,

  DPC_CKILL,

  DPC_CLIST,
  DPC_CREPLY,
} DPCCmd;

typedef struct DPCMsgProcInfo
{
  DPId id;
  DPChar name[CMD_NAME_MAX];
  DPPid pid;
  DPDword restarts;
  DPProcState state;
} DPCMsgProcInfo;

typedef union DPCMsgData
{
  DPChar text[CMD_ARG_MAX];
  DPCMsgProcInfo pinfo;
} DPCMsgData;

typedef struct DPCMsgText
{
  DPLong cmd;
  DPChar text[CMD_ARG_MAX];
} DPCMsgText;

typedef struct DPCMsgInfo
{
  DPLong cmd;
  DPCMsgProcInfo pinfo;
} DPCMsgInfo;

typedef struct DPCMsgEmpty
{
  DPLong cmd;
} DPCMsgEmpty;

typedef struct DPCMsgAny
{
  DPLong cmd;
  DPCMsgData data;
} DPCMsgAny;

COMPILE_ASSERT (SIZEOF (DPCMsgText) == SIZEOF (DPCMsgAny));

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API const DPChar *dpcCommandStr (DPCCmd cmd);

#endif
