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

#include "app/proc.h"
#include "bus/init.h"
#include "bus/registry.h"
#include "core/def.h"
#include "core/error.h"
#include "plat/common.h"
#include "plat/logger.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpProcStart (DPId id)
{
  CHECK_INIT_RETURN ();

  DP_INFO ("Start requested for process: %d", id);

  DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcPtrInfoEx (id, &info));

  DPProcStateEx state;
  CHECK_STATUS_RETURN (dpProcInfoExGetState (info, &state));

  switch (state)
    {
    case DP_RUNNING_EX:
      {
        DP_DEBUG ("dpProcStart: Process %u: Already running", id);

        return DP_OK;
      }

    case DP_STOPPING_EX:
    case DP_RESTARTING_EX:
      {
        CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_RUNNING_EX));

        DP_DEBUG ("dpProcStart: Process %u: Canceled stop/restart", id);

        return DP_OK;
      }

    case DP_STARTING_EX:
      {
        DP_DEBUG ("dpProcStart: Process %u: Already starting", id);

        return DP_OK;
      }

    case DP_EXITED_EX:
    case DP_SIGNALED_EX:
    case DP_RESTARTED_EX:
      {
        DP_DEBUG ("dpProcStart: Process %u: Already scheduled for start", id);

        return DP_OK;
      }

    case DP_ERRORED_EX:
    case DP_STOPPED_EX:
      {
        CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_STARTING_EX));

        DP_DEBUG ("dpProcStart: Process %u: Scheduled for start", id);

        return DP_OK;
      }
    }

  SET_ERROR_RETURN (DP_EUNEXPECTED, "Reached end of dpProcStart");
}

DPStatus
dpProcStop (DPId id)
{
  CHECK_INIT_RETURN ();

  DP_INFO ("Stop requested for process: %d", id);

  DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcPtrInfoEx (id, &info));

  DPProcStateEx state;
  CHECK_STATUS_RETURN (dpProcInfoExGetState (info, &state));

  switch (state)
    {
    case DP_STOPPED_EX:
      {
        DP_DEBUG ("dpProcStop: Process %u: Already stopped", id);

        return DP_OK;
      }

    case DP_STOPPING_EX:
      {
        DP_DEBUG ("dpProcStop: Process %u: Already stopping", id);

        return DP_OK;
      }

    case DP_RESTARTED_EX:
    case DP_STARTING_EX:
    case DP_EXITED_EX:
    case DP_SIGNALED_EX:
    case DP_ERRORED_EX:
      {
        CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_STOPPED_EX));

        DP_DEBUG ("dpProcStop: Process %u: Stopped", id);

        return DP_OK;
      }

    case DP_RESTARTING_EX:
    case DP_RUNNING_EX:
      {
        CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_STOPPING_EX));

        DP_DEBUG ("dpProcStop: Process %u: Scheduled for stop", id);

        return DP_OK;
      }
    }

  SET_ERROR_RETURN (DP_EUNEXPECTED, "Reached end of dpProcStop");
}

DPStatus
dpProcRestart (DPId id)
{
  CHECK_INIT_RETURN ();

  DP_INFO ("Restart requested for process: %d", id);

  DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcPtrInfoEx (id, &info));

  DPProcStateEx state;
  CHECK_STATUS_RETURN (dpProcInfoExGetState (info, &state));

  switch (state)
    {
    case DP_STARTING_EX:
      {
        DP_DEBUG ("dpProcRestart: Process %u: Already starting", id);

        return DP_OK;
      }
    case DP_RESTARTING_EX:
      {
        DP_DEBUG ("dpProcRestart: Process %u: Already restarting", id);

        return DP_OK;
      }

    case DP_EXITED_EX:
    case DP_SIGNALED_EX:
    case DP_RESTARTED_EX:
      {
        DP_DEBUG ("dpProcRestart: Process %u: Already scheduled for start",
                  id);

        return DP_OK;
      }

    case DP_STOPPED_EX:
    case DP_ERRORED_EX:
      {
        CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_STARTING_EX));

        DP_DEBUG ("dpProcRestart: Process %u: Scheduled for start", id);

        return DP_OK;
      }

    case DP_STOPPING_EX:
    case DP_RUNNING_EX:
      {
        CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_RESTARTING_EX));

        DP_DEBUG ("dpProcRestart: Process %u: Scheduled for restart", id);

        return DP_OK;
      }
    }

  SET_ERROR_RETURN (DP_EUNEXPECTED, "Reached end of dpProcRestart");
}
