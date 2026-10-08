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

#include "core/info.h"
#include "core/error.h"
#include "plat/alloc.h"
#include "plat/io.h"
#include "plat/logger.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct DPSProcInfo
{
  DPProcState state;
  DPDword restarts;
  DPPid pid;
};

struct DPSProcInfoEx
{
  DPProcStateEx state;
  DPDword restarts;
  DPPid pid;

  DPQword exitat;
  DPFd fdin;
  DPFd fdout;
  DPFd fderr;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpProcInfoDestroy (DPProcInfo *info)
{
  CHECK_INFO_RETURN (info);

  dpFree (*info);

  *info = NULL;

  return DP_OK;
}

DPStatus
dpProcInfoGetState (const DPProcInfo *info, DPProcState *state)
{
  CHECK_INFO_RETURN (info);
  CHECK_NULL_RETURN (state);

  *state = (*info)->state;

  return DP_OK;
}

DPStatus
dpProcInfoGetRestarts (const DPProcInfo *info, DPDword *restarts)
{
  CHECK_INFO_RETURN (info);
  CHECK_NULL_RETURN (restarts);

  *restarts = (*info)->restarts;

  return DP_OK;
}

DPStatus
dpProcInfoGetPid (const DPProcInfo *info, DPPid *pid)
{
  CHECK_INFO_RETURN (info);
  CHECK_NULL_RETURN (pid);

  *pid = (*info)->pid;

  return DP_OK;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

const DPPid DP_PID_INVALID = -1;

DPBool
dpIsPidValid (DPPid pid)
{
  return pid != DP_PID_INVALID;
}

DPStatus
dpProcInfoInfer (DPProcInfo *dst, const DPProcInfoEx *src)
{
  CHECK_INFO_RETURN (src);
  CHECK_NULL_RETURN (dst);

  *dst = (DPProcInfo)dpMalloc (SIZEOF (struct DPSProcInfo));
  CHECK_ALLOC_RETURN (*dst);

  switch ((*src)->state)
    {
    case DP_STOPPED_EX:
    case DP_ERRORED_EX:
      // Only these states mean that there won't be any auto-restart.
      (*dst)->state = DP_STOPPED;
      break;

    default:
      (*dst)->state = DP_RUNNING;
      break;
    }
  (*dst)->restarts = (*src)->restarts;
  (*dst)->pid = (*src)->pid;

  return DP_OK;
}

DPStatus
dpProcInfoExInit (DPProcInfoEx *info)
{
  CHECK_NULL_RETURN (info);

  *info = (DPProcInfoEx)dpMalloc (SIZEOF (struct DPSProcInfoEx));
  CHECK_ALLOC_RETURN (*info);

  // Apply defaults
  (*info)->state = DP_STOPPED_EX;
  (*info)->restarts = 0;
  (*info)->pid = DP_PID_INVALID;
  (*info)->exitat = 0;
  (*info)->fdin = DP_FD_INVALID;
  (*info)->fdout = DP_FD_INVALID;
  (*info)->fderr = DP_FD_INVALID;

  return DP_OK;
}

DPStatus
dpProcInfoExDestroy (DPProcInfoEx *info)
{
  CHECK_INFO_RETURN (info);

  dpFree (*info);

  *info = NULL;

  return DP_OK;
}

DPStatus
dpProcInfoExSetState (DPProcInfoEx *info, DPProcStateEx state)
{
  CHECK_INFO_RETURN (info);

  (*info)->state = state;

  return DP_OK;
}

DPStatus
dpProcInfoExGetState (const DPProcInfoEx *info, DPProcStateEx *state)
{
  CHECK_INFO_RETURN (info);
  CHECK_NULL_RETURN (state);

  *state = (*info)->state;

  return DP_OK;
}

DPStatus
dpProcInfoExSetRestarts (DPProcInfoEx *info, DPDword restarts)
{
  CHECK_INFO_RETURN (info);

  (*info)->restarts = restarts;

  return DP_OK;
}

DPStatus
dpProcInfoExGetRestarts (const DPProcInfoEx *info, DPDword *restarts)
{
  CHECK_INFO_RETURN (info);
  CHECK_NULL_RETURN (restarts);

  *restarts = (*info)->restarts;

  return DP_OK;
}

DPStatus
dpProcInfoExIncRestarts (const DPProcInfoEx *info)
{
  CHECK_INFO_RETURN (info);

  ++((*info)->restarts);

  return DP_OK;
}

DPStatus
dpProcInfoExSetPid (DPProcInfoEx *info, DPPid pid)
{
  CHECK_INFO_RETURN (info);

  (*info)->pid = pid;

  return DP_OK;
}

DPStatus
dpProcInfoExGetPid (const DPProcInfoEx *info, DPPid *pid)
{
  CHECK_INFO_RETURN (info);
  CHECK_NULL_RETURN (pid);

  *pid = (*info)->pid;

  return DP_OK;
}

DPStatus
dpProcInfoExSetExitAt (DPProcInfoEx *info, DPQword sec)
{
  CHECK_INFO_RETURN (info);

  (*info)->exitat = sec;

  return DP_OK;
}

DPStatus
dpProcInfoExGetExitAt (const DPProcInfoEx *info, DPQword *sec)
{
  CHECK_INFO_RETURN (info);
  CHECK_NULL_RETURN (sec);

  *sec = (*info)->exitat;

  return DP_OK;
}

DPStatus
dpProcInfoExSetFdIn (DPProcInfoEx *info, DPFd fdin)
{
  CHECK_INFO_RETURN (info);

  (*info)->fdin = fdin;

  return DP_OK;
}

DPStatus
dpProcInfoExGetFdIn (const DPProcInfoEx *info, DPFd *fdin)
{
  CHECK_INFO_RETURN (info);
  CHECK_NULL_RETURN (fdin);

  *fdin = (*info)->fdin;

  return DP_OK;
}

DPStatus
dpProcInfoExSetFdOut (DPProcInfoEx *info, DPPid fdout)
{
  CHECK_INFO_RETURN (info);

  (*info)->fdout = fdout;

  return DP_OK;
}

DPStatus
dpProcInfoExGetFdOut (const DPProcInfoEx *info, DPPid *fdout)
{
  CHECK_INFO_RETURN (info);
  CHECK_NULL_RETURN (fdout);

  *fdout = (*info)->fdout;

  return DP_OK;
}

DPStatus
dpProcInfoExSetFdErr (DPProcInfoEx *info, DPPid fderr)
{
  CHECK_INFO_RETURN (info);

  (*info)->fderr = fderr;

  return DP_OK;
}

DPStatus
dpProcInfoExGetFdErr (const DPProcInfoEx *info, DPPid *fderr)
{
  CHECK_INFO_RETURN (info);
  CHECK_NULL_RETURN (fderr);

  *fderr = (*info)->fderr;

  return DP_OK;
}

DPStatus
dpProcInfoExClone (DPProcInfoEx *dst, const DPProcInfoEx *src)
{
  CHECK_INFO_RETURN (src);
  CHECK_NULL_RETURN (dst);

  CHECK_STATUS_RETURN (dpProcInfoExInit (dst));

  (*dst)->state = (*src)->state;
  (*dst)->restarts = (*src)->restarts;
  (*dst)->pid = (*src)->pid;
  (*dst)->exitat = (*src)->exitat;
  (*dst)->fdin = (*src)->fdin;
  (*dst)->fdout = (*src)->fdout;
  (*dst)->fderr = (*src)->fderr;

  return DP_OK;
}

DPStatus
dpProcInfoExPrint (const DPProcInfoEx *info)
{
  if (info == NULL || *info == NULL)
    {
      DP_INFO ("ProcInfo: (NULL)");
      return DP_OK;
    }

  DP_INFO ("ProcInfo:");
  DP_INFO ("- state: %u", (*info)->state);
  DP_INFO ("- restarts: %u", (*info)->restarts);
  DP_INFO ("- pid: %d", (*info)->pid);
  DP_INFO ("- exitat: %lu", (*info)->exitat);
  DP_INFO ("- fdin: %d", (*info)->fdin);
  DP_INFO ("- fdout: %d", (*info)->fdout);
  DP_INFO ("- fderr: %d", (*info)->fderr);

  return DP_OK;
}
