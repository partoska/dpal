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

#ifndef _CORE_INFO_H_
#define _CORE_INFO_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "core/error.h"
#include "plat/common.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define CHECK_INFO_RETURN(info)                                               \
  do                                                                          \
    {                                                                         \
      CHECK_NULL_RETURN (info);                                               \
      CHECK_NULL_RETURN (*info);                                              \
    }                                                                         \
  while (FALSE)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct DPSProcInfoEx *DPProcInfoEx;

typedef enum DPEProcStateEx
{
  /* Process was manually stopped and is NOT expected to be auto-restarted. */
  DP_STOPPED_EX = 0x00,

  /* Process exited and is expected to be auto-restarted. */
  DP_EXITED_EX = 0x01,

  /* Process was signaled and is expected to be auto-restarted. */
  DP_SIGNALED_EX = 0x02,

  /* Process behaved abnormally and was stopped. It is NOT expected to be
     auto-restarted. */
  DP_ERRORED_EX = 0x03,

  /* Process was stopped because it was manually scheduled for restart. */
  DP_RESTARTED_EX = 0x04,

  /* Process is manually scheduled for start. */
  DP_STARTING_EX = 0x05,

  /* Process is running normally. */
  DP_RUNNING_EX = 0x10,

  /* Process is running but was manually scheduled for stop. */
  DP_STOPPING_EX = 0x11,

  /* Process is running but was manually scheduled for restart. */
  DP_RESTARTING_EX = 0x12,
} DPProcStateEx;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API DPStatus dpProcInfoDestroy (DPProcInfo *info);
DP_API DPStatus dpProcInfoGetState (const DPProcInfo *info,
                                    DPProcState *state);
DP_API DPStatus dpProcInfoGetRestarts (const DPProcInfo *info,
                                       DPDword *restarts);
DP_API DPStatus dpProcInfoGetPid (const DPProcInfo *info, DPPid *pid);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API const DPPid DP_PID_INVALID;
DP_API DPBool dpIsPidValid (DPPid pid);

DP_API DPStatus dpProcInfoInfer (DPProcInfo *dst, const DPProcInfoEx *src);

DP_API DPStatus dpProcInfoExInit (DPProcInfoEx *info);
DP_API DPStatus dpProcInfoExDestroy (DPProcInfoEx *info);
DP_API DPStatus dpProcInfoExSetState (DPProcInfoEx *info, DPProcStateEx state);
DP_API DPStatus dpProcInfoExGetState (const DPProcInfoEx *info,
                                      DPProcStateEx *state);

DP_API DPStatus dpProcInfoExSetRestarts (DPProcInfoEx *info, DPDword restarts);
DP_API DPStatus dpProcInfoExGetRestarts (const DPProcInfoEx *info,
                                         DPDword *restarts);
DP_API DPStatus dpProcInfoExIncRestarts (const DPProcInfoEx *info);
DP_API DPStatus dpProcInfoExSetPid (DPProcInfoEx *info, DPPid pid);
DP_API DPStatus dpProcInfoExGetPid (const DPProcInfoEx *info, DPPid *pid);
DP_API DPStatus dpProcInfoExSetExitAt (DPProcInfoEx *info, DPQword sec);
DP_API DPStatus dpProcInfoExGetExitAt (const DPProcInfoEx *info,
                                       DPQword *sec);
DP_API DPStatus dpProcInfoExSetFdIn (DPProcInfoEx *info, DPFd fdin);
DP_API DPStatus dpProcInfoExGetFdIn (const DPProcInfoEx *info, DPFd *fdin);
DP_API DPStatus dpProcInfoExSetFdOut (DPProcInfoEx *info, DPFd fdout);
DP_API DPStatus dpProcInfoExGetFdOut (const DPProcInfoEx *info, DPFd *fdout);
DP_API DPStatus dpProcInfoExSetFdErr (DPProcInfoEx *info, DPFd fderr);
DP_API DPStatus dpProcInfoExGetFdErr (const DPProcInfoEx *info, DPFd *fderr);

DP_API DPStatus dpProcInfoExClone (DPProcInfoEx *dst, const DPProcInfoEx *src);
DP_API DPStatus dpProcInfoExPrint (const DPProcInfoEx *info);

#endif
