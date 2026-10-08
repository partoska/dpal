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

#include "bus/init.h"
#include "bus/registry.h"
#include "core/attr.h"
#include "core/def.h"
#include "core/error.h"
#include "core/info.h"
#include "core/log.h"
#include "plat/common.h"
#include "plat/env.h"
#include "plat/filesys.h"
#include "plat/fork.h"
#include "plat/io.h"
#include "plat/logger.h"
#include "plat/select.h"
#include "plat/time.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define FIFTY_MS (50 * 1000)
#define ONE_SECOND (1000 * 1000)
#define TICK_TIME ONE_SECOND

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static volatile DPByte init = FALSE;

static DPFdSet readFds;
static DPFdSet writeFds;
static DPFdSet exceptFds;
static DPInt maxFd;

static void (*execBefore) (DPId id) = NULL;
static void (*execAfterExit) (DPId id, DPInt code) = NULL;
static void (*execAfterSig) (DPId id, DPInt sig) = NULL;
static void (*execFail) (DPId id) = NULL;

// ===============================================================
// Resurrect
// ===============================================================

static DPStatus
procResurrectVisitor (DPId id)
{
  DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcPtrInfoEx (id, &info));

  DPProcStateEx state;
  CHECK_STATUS_RETURN (dpProcInfoExGetState (info, &state));
  switch (state)
    {
    case DP_EXITED_EX:
    case DP_SIGNALED_EX:
    case DP_RESTARTED_EX:
      // Process should be restarted.
      break;

    default:
      return DP_OK;
    }

  // Check whether restart delay passed.
  const DPProcAttr *attr;
  CHECK_STATUS_RETURN (dpProcRefProps (id, NULL, &attr, NULL));

  DPDword sec;
  CHECK_STATUS_RETURN (dpProcAttrGetRestartSec (attr, &sec));
  if (sec > 0)
    {
      DPQword exit;
      CHECK_STATUS_RETURN (dpProcInfoExGetExitAt (info, &exit));

      DPQword waited = dpTime () - exit;
      if (waited <= (DPQword)sec)
        {
          DP_DSLOW ("procResurrectVisitor: Process %u: Restart delayed: %lu",
                    id, (DPQword)sec - waited);
          return DP_OK;
        }
    }

  DP_DSLOW ("procResurrectVisitor: Process %u: Schedule restart", id);

  CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_STARTING_EX));
  CHECK_STATUS_RETURN (dpProcInfoExIncRestarts (info));

  return DP_OK;
}

// ===============================================================
// Stop
// ===============================================================

static DPStatus
procStop (DPId id)
{
  DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcPtrInfoEx (id, &info));

  DPPid pid;
  CHECK_STATUS_RETURN (dpProcInfoExGetPid (info, &pid));

  CHECK_RETURN (dpKill (pid, DP_SIGTERM) == 0, DP_EPROCESS_STOP);

  DP_DSLOW ("procStop: Process %u: PID: %d: Killed /w SIGTERM", id, pid);

  return DP_OK;
}

static DPStatus
procStopVisitor (DPId id)
{
  DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcPtrInfoEx (id, &info));

  DPProcStateEx state;
  CHECK_STATUS_RETURN (dpProcInfoExGetState (info, &state));
  switch (state)
    {
    case DP_STOPPING_EX:
    case DP_RESTARTING_EX:
      // Process is meant to be stopped.
      break;

    default:
      return DP_OK;
    }

  DP_DEBUG ("procStopVisitor: Process %u: Stopping", id);

  CHECK_STATUS_RETURN (procStop (id));

  return DP_OK;
}

// ===============================================================
// Start
// ===============================================================

static DPStatus
procStart (DPId id)
{
  const DPProcDef *def;
  const DPProcAttr *attr;
  CHECK_STATUS_RETURN (dpProcRefProps (id, &def, &attr, NULL));

  const DPChar *app;
  CHECK_STATUS_RETURN (dpProcDefRefApp (def, &app));

  const DPChar *name;
  CHECK_STATUS_RETURN (dpProcDefRefName (def, &name));

  const DPChar *dir;
  CHECK_STATUS_RETURN (dpProcDefRefDir (def, &dir));

  const DPChar *const *argv;
  CHECK_STATUS_RETURN (dpProcDefRefArgv (def, &argv));

  const DPChar *const *envs;
  CHECK_STATUS_RETURN (dpProcDefRefEnvs (def, &envs));

  DPSize envc;
  CHECK_STATUS_RETURN (dpProcDefGetEnvc (def, &envc));

#if DEBUG_SLOW
  DPSize argc;
  CHECK_STATUS_RETURN (dpProcDefGetArgc (def, &argc));

  DP_DSLOW ("procStart: Exec: %s", app);
  DP_DSLOW ("procStart: Args (%u):", argc);
  for (DPSize i = 0; i < argc; ++i)
    {
      DP_DSLOW ("[%u] %s", i, argv[i]);
    }

  DP_DSLOW ("procStart: Envs (%u):", envc);
  for (DPSize i = 0; i < envc; ++i)
    {
      DP_DSLOW ("[%u] %s", i, envs[i]);
    }
#endif

  DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcPtrInfoEx (id, &info));

  DPFd pipeOut[2];
  CHECK_RETURN (dpPipe (pipeOut) == 0, DP_EPIPE);

  DPFd pipeErr[2];
  CHECK_RETURN (dpPipe (pipeErr) == 0, DP_EPIPE);

  DPFd pipeIn[2];
  CHECK_RETURN (dpPipe (pipeIn) == 0, DP_EPIPE);

  DPPid pid = dpFork ();
  CHECK_RETURN (pid >= 0, DP_EPROCESS_CREATE);
  if (pid == 0)
    {
      // Child process -> We have to always exit on error

      // Disallow any further dpal library calls by setting deinit state
      init = FALSE;

      // Invoke callback before exec
      if (execBefore != NULL)
        {
          execBefore (id);
        }

      do
        {
          // Close the read ends of the pipes
          if (dpClose (pipeOut[0]) == -1)
            {
              break;
            }
          if (dpClose (pipeErr[0]) == -1)
            {
              break;
            }

          // Redirect stdout and stderr to the pipes
          if (dpDup2 (pipeOut[1], DP_FD_STDOUT) == -1)
            {
              break;
            }
          if (dpDup2 (pipeErr[1], DP_FD_STDERR) == -1)
            {
              break;
            }

          // After redirection, pipe ends are no longer needed
          if (dpClose (pipeOut[1]) == -1)
            {
              break;
            }
          if (dpClose (pipeErr[1]) == -1)
            {
              break;
            }

          // Close the write end of the stdin pipe
          if (dpClose (pipeIn[1]) == -1)
            {
              break;
            }

          // Redirect stdin to the pipe
          if (dpDup2 (pipeIn[0], DP_FD_STDIN) == -1)
            {
              break;
            }

          // After redirection, the pipe end is no longer needed
          if (dpClose (pipeIn[0]) == -1)
            {
              break;
            }

          // Switch to target directory
          if (dir != NULL && dpChdir (dir) != 0)
            {
              break;
            }

          // Set environment variables
          for (DPSize i = 0; i < envc; ++i)
            {
              if (dpPutenv ((DPChar *)envs[i]) != 0)
                {
                  // Invoke callback before exec
                  if (execFail != NULL)
                    {
                      execFail (id);
                    }

                  dpExit (DP_EPROCESS_CREATE);
                }
            }

          // Execute the child process
          dpExecvp (app, (DPChar *const *)argv);
        }
      while (FALSE);

      // Invoke callback before exec
      if (execFail != NULL)
        {
          execFail (id);
        }

      // Failed to execute the child process
      dpExit (DP_EPROCESS_CREATE);
    }
  else
    {
      // Parent process

      // Close the write ends of the pipes with stdout and stderr
      CHECK_REPORT (dpClose (pipeOut[1]) == 0, DP_ECLOSE);
      CHECK_REPORT (dpClose (pipeErr[1]) == 0, DP_ECLOSE);

      // Close the read end of the pipe with stdin
      CHECK_REPORT (dpClose (pipeIn[0]) == 0, DP_ECLOSE);

      // Update process info
      CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_RUNNING_EX));
      CHECK_STATUS_RETURN (dpProcInfoExSetPid (info, pid));
      CHECK_STATUS_RETURN (dpProcInfoExSetExitAt (info, 0));
      CHECK_STATUS_RETURN (dpProcInfoExSetFdOut (info, pipeOut[0]));
      CHECK_STATUS_RETURN (dpProcInfoExSetFdErr (info, pipeErr[0]));
      CHECK_STATUS_RETURN (dpProcInfoExSetFdIn (info, pipeIn[1]));

      DPLog *out;
      DPLog *err;
      CHECK_STATUS_RETURN (dpProcPtrLog (id, &out, &err));
      CHECK_STATUS_RETURN (dpLogStart (out));
      CHECK_STATUS_RETURN (dpLogStart (err));
    }

  return DP_OK;
}

static DPStatus
procStartVisitor (DPId id)
{
  const DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcRefInfoEx (id, &info));

  DPProcStateEx state;
  CHECK_STATUS_RETURN (dpProcInfoExGetState (info, &state));
  switch (state)
    {
    case DP_STARTING_EX:
      // Process should be started.
      break;

    default:
      return DP_OK;
    }

#if _DEBUG
  DPDword restarts = 0;
  CHECK_STATUS_RETURN (dpProcInfoExGetRestarts (info, &restarts));
  DP_DEBUG ("procStartVisitor: Process %u: Starting (restarts: %u)", id,
            restarts);
#endif

  CHECK_STATUS_RETURN (procStart (id));

  return DP_OK;
}

// ===============================================================
// File descriptor - Collect
// ===============================================================

static DPBool
procFdSet (DPFd fd)
{
  if (!dpIsFdValid (fd))
    {
      return FALSE;
    }

  dpFdsSet (fd, &readFds);
  dpFdsSet (fd, &exceptFds);
  maxFd = MAX (maxFd, fd + 1);

  return TRUE;
}

static DPStatus
procFdVisitor (DPId id)
{
  const DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcRefInfoEx (id, &info));

  DPProcStateEx state;
  CHECK_STATUS_RETURN (dpProcInfoExGetState (info, &state));
  switch (state)
    {
    case DP_RUNNING_EX:
    case DP_STOPPING_EX:
    case DP_RESTARTING_EX:
      // Process can produce data in these states.
      break;

    default:
      return DP_OK;
    }

  DPFd fd;

  // Add stdout to the read and except sets
  CHECK_STATUS_RETURN (dpProcInfoExGetFdOut (info, &fd));
  if (procFdSet (fd))
    {
      DP_DSLOW ("procFdsVisitor: Process %u: Stdout %d: Adding to read set",
                id, fd);
    }
  else
    {
      DP_DSLOW ("procFdsVisitor: Process %u: Stdout already closed", id, fd);
    }

  // Add stderr to the read and except sets
  CHECK_STATUS_RETURN (dpProcInfoExGetFdErr (info, &fd));
  if (procFdSet (fd))
    {
      DP_DSLOW ("procFdsVisitor: Process %u: Stderr %d: Adding to read set",
                id, fd);
    }
  else
    {
      DP_DSLOW ("procFdsVisitor: Process %u: Stderr already closed", id, fd);
    }

  return DP_OK;
}

// ===============================================================
// File descriptor - Read
// ===============================================================

static DPStatus
procFdRead (DPId id, DPFd fd, DPProcInfoEx *info,
            DPStatus setFdFn (DPProcInfoEx *, DPFd), const DPLog *log)
{
#if (!defined(DEBUG_SLOW)) || (DEBUG_SLOW == 0)
  // Variable unused in non-verbose builds
  UNUSED (id);
#endif

  // Do not attempt to read from closed descriptors.
  if (!dpIsFdValid (fd))
    {
      return DP_OK;
    }

  if (!dpFdsIsSet (fd, &readFds))
    {
      return DP_OK;
    }

  DPChar buffer[1024];
  DPSSize bytes = dpRead (fd, buffer, SIZEOF (buffer));
  CHECK_RETURN (bytes >= 0, DP_EREAD);

  // Attempt to read bytes from file.
  if (bytes > 0)
    {
      DP_DSLOW ("procFdRead: Process %u: Fd %d: Read: %d", id, fd, bytes);

      CHECK_STATUS_RETURN (dpLog (log, buffer, bytes));

      return DP_OK;
    }

  // File is now closed (no bytes were read).
  DP_DSLOW ("procFdRead: Process %u: Fd %d: Close", id, fd);

  CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
  CHECK_STATUS_RETURN (setFdFn (info, DP_FD_INVALID));

  return DP_OK;
}

static DPStatus
procFdReadVisitor (DPId id)
{
  DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcPtrInfoEx (id, &info));

  const DPLog *out;
  const DPLog *err;
  CHECK_STATUS_RETURN (dpProcRefLog (id, &out, &err));

  DPFd fd;
  CHECK_STATUS_RETURN (dpProcInfoExGetFdOut (info, &fd));
  CHECK_STATUS_REPORT (procFdRead (id, fd, info, dpProcInfoExSetFdOut, out));

  CHECK_STATUS_RETURN (dpProcInfoExGetFdErr (info, &fd));
  CHECK_STATUS_REPORT (procFdRead (id, fd, info, dpProcInfoExSetFdErr, err));

  return DP_OK;
}

// ===============================================================
// Exit
// ===============================================================

static DPStatus
procCleanUp (DPProcInfoEx *info)
{
  CHECK_STATUS_RETURN (dpProcInfoExSetPid (info, DP_PID_INVALID));

  DPFd fd;

  CHECK_STATUS_RETURN (dpProcInfoExGetFdOut (info, &fd));
  CHECK_REPORT (!dpIsFdValid (fd) || dpClose (fd) == 0, DP_ECLOSE);
  CHECK_STATUS_RETURN (dpProcInfoExSetFdOut (info, DP_FD_INVALID));

  CHECK_STATUS_RETURN (dpProcInfoExGetFdErr (info, &fd));
  CHECK_REPORT (!dpIsFdValid (fd) || dpClose (fd) == 0, DP_ECLOSE);
  CHECK_STATUS_RETURN (dpProcInfoExSetFdErr (info, DP_FD_INVALID));

  CHECK_STATUS_RETURN (dpProcInfoExGetFdIn (info, &fd));
  CHECK_REPORT (!dpIsFdValid (fd) || dpClose (fd) == 0, DP_ECLOSE);
  CHECK_STATUS_RETURN (dpProcInfoExSetFdIn (info, DP_FD_INVALID));

  return DP_OK;
}

static DPStatus
procAwaitVisitor (DPId id)
{
  DPProcInfoEx *info;
  CHECK_STATUS_RETURN (dpProcPtrInfoEx (id, &info));

  DPProcStateEx state;
  CHECK_STATUS_RETURN (dpProcInfoExGetState (info, &state));
  switch (state)
    {
    case DP_RUNNING_EX:
    case DP_STOPPING_EX:
    case DP_RESTARTING_EX:
      // Process can exit/be signaled in these states.
      break;

    default:
      return DP_OK;
    }

  DPPid pid;
  CHECK_STATUS_RETURN (dpProcInfoExGetPid (info, &pid));
  CHECK_RETURN (dpIsPidValid (pid), DP_EPID);

  DPInt status;
  DPPid result = dpWaitpid (pid, &status, DP_WNOHANG);
  CHECK_RETURN (result >= 0, DP_EWAIT);

  if (result == 0)
    {
      DP_DSLOW ("procAwaitVisitor: Process %d: PID %d: Not exited yet", id,
                pid);

      return DP_OK;
    }

  // All right. It seems that the process has exited.
  DPLog *out;
  DPLog *err;
  CHECK_STATUS_RETURN (dpProcPtrLog (id, &out, &err));
  CHECK_STATUS_RETURN (dpLogStop (out));
  CHECK_STATUS_RETURN (dpLogStop (err));

  if (DP_WIFEXITED (status))
    {
      DPInt code = DP_WEXITSTATUS (status);

      DP_DSLOW ("procAwaitVisitor: Process %u: PID %d: Exited with status: %d",
                id, pid, code);

      CHECK_STATUS_RETURN (procCleanUp (info));

      if (execAfterExit != NULL)
        {
          execAfterExit (id, code);
        }

      switch (state)
        {
        case DP_STOPPING_EX:
          {
            CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_STOPPED_EX));
            DP_DEBUG ("procAwaitVisitor: Process %u: PID %d: Stopped", id,
                      pid);

            return DP_OK;
          }
        case DP_RESTARTING_EX:
          {
            CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_RESTARTED_EX));
            DP_DEBUG ("procAwaitVisitor: Process %u: PID %d: Schedule restart",
                      id, pid);

            return DP_OK;
          }
        case DP_RUNNING_EX:
        default:
          {
            /* Abnormal exit */
            CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_EXITED_EX));
            DP_DEBUG ("procAwaitVisitor: Process %u: PID %d: Exited", id, pid);

            CHECK_STATUS_RETURN (dpProcInfoExSetExitAt (info, dpTime ()));

            return DP_OK;
          }
        }
    }
  else if (DP_WIFSIGNALED (status))
    {
      DPInt sig = DP_WTERMSIG (status);

      DP_DSLOW ("procAwaitVisitor: Process %u: PID %d: Terminated by "
                "signal: %d",
                id, pid, sig);

      CHECK_STATUS_RETURN (procCleanUp (info));

      if (execAfterSig != NULL)
        {
          execAfterSig (id, sig);
        }

      switch (state)
        {
        case DP_STOPPING_EX:
          {
            CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_STOPPED_EX));
            DP_DEBUG ("procAwaitVisitor: Process %u: PID %d: Stopped", id,
                      pid);

            return DP_OK;
          }
        case DP_RESTARTING_EX:
          {
            CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_RESTARTED_EX));
            DP_DEBUG ("procAwaitVisitor: Process %u: PID %d: Schedule "
                      "restart",
                      id, pid);

            return DP_OK;
          }
        case DP_RUNNING_EX:
        default:
          {
            /* Abnormal signal */
            CHECK_STATUS_RETURN (dpProcInfoExSetState (info, DP_SIGNALED_EX));
            DP_DEBUG ("procAwaitVisitor: Process %u: PID %d: Signaled", id,
                      pid);

            CHECK_STATUS_RETURN (dpProcInfoExSetExitAt (info, dpTime ()));

            return DP_OK;
          }
        }
    }

  // This should never happen ...
  SET_ERROR_RETURN (DP_EUNEXPECTED, "End of procAwaitVisitor");
}

// ===============================================================
// Exec callbacks
// ===============================================================

DPStatus
dpExecBefore (void (*handler) (DPId id))
{
  CHECK_INIT_RETURN ();

  execBefore = handler;

  return DP_OK;
}

DPStatus
dpExecFail (void (*handler) (DPId id))
{
  CHECK_INIT_RETURN ();

  execFail = handler;

  return DP_OK;
}

DPStatus
dpExecAfterExit (void (*handler) (DPId id, DPInt code))
{
  CHECK_INIT_RETURN ();

  execAfterExit = handler;

  return DP_OK;
}

DPStatus
dpExecAfterSig (void (*handler) (DPId id, DPInt sig))
{
  CHECK_INIT_RETURN ();

  execAfterSig = handler;

  return DP_OK;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpInit (void)
{
  CHECK_DOUBLE_INIT_RETURN ();

  DP_INFO (
      "==================================================================");
  DP_INFO ("Daemon Pal Library v" DP_VERSION
           ", <https://lab.partoska.com/dpal>");
  DP_INFO ("Copyright (C) 2024 Fabrika Charvat s.r.o. All rights reserved.");
  DP_INFO (
      "==================================================================");

  CHECK_STATUS_RETURN (dpRegInit ());

  CHECK_RETURN (dpFdsInit (&readFds) == 0, DP_EALLOC);
  CHECK_RETURN (dpFdsInit (&writeFds) == 0, DP_EALLOC);
  CHECK_RETURN (dpFdsInit (&exceptFds) == 0, DP_EALLOC);

  execBefore = NULL;
  execAfterExit = NULL;
  execAfterSig = NULL;
  execFail = NULL;

  init = TRUE;

  DP_INFO ("Process manager successfully initialized");

  return DP_OK;
}

DPStatus
dpTick (void)
{
  CHECK_INIT_RETURN ();

  CHECK_STATUS_REPORT (dpRegEach (procResurrectVisitor, dpVisitContinueCb));
  CHECK_STATUS_REPORT (dpRegEach (procStartVisitor, dpVisitContinueCb));
  CHECK_STATUS_REPORT (dpRegEach (procStopVisitor, dpVisitContinueCb));

  DPInt fds;
  do
    {
      fds = 0;
      maxFd = 0;

      dpFdsZero (&readFds);
      dpFdsZero (&writeFds);
      dpFdsZero (&exceptFds);

      CHECK_STATUS_REPORT (dpRegEach (procFdVisitor, dpVisitContinueCb));

      if (maxFd > 0)
        {
          DP_DSLOW ("dpInit: Waiting for select");
          DPUSeconds timeout = 0;
          fds = dpSelect (maxFd, &readFds, NULL, &exceptFds, &timeout);
          DP_DSLOW ("dpInit: Select fds: %d", fds);
          if (fds < 0)
            {
              dpSetError (DP_ESELECT,
                          "Error while waiting for file descriptors!");
              dpPrintError (NULL);
            }
          else if (fds > 0)
            {
              CHECK_STATUS_REPORT (
                  dpRegEach (procFdReadVisitor, dpVisitContinueCb));
            }
          else
            {
              DP_DSLOW ("dpInit: Select timeouted");
            }
        }
    }
  while (fds > 0);

  CHECK_STATUS_REPORT (dpRegEach (procAwaitVisitor, dpVisitContinueCb));

  return DP_OK;
}

DPStatus
dpDestroy (void)
{
  CHECK_INIT_RETURN ();

  DP_DEBUG ("De-initialization requested");

  CHECK_STATUS_REPORT (dpRegDestroy ());

  execBefore = NULL;
  execAfterExit = NULL;
  execAfterSig = NULL;
  execFail = NULL;

  CHECK_REPORT (dpFdsDestroy (&exceptFds) == 0, DP_ENULL);
  CHECK_REPORT (dpFdsDestroy (&writeFds) == 0, DP_ENULL);
  CHECK_REPORT (dpFdsDestroy (&readFds) == 0, DP_ENULL);

  init = FALSE;

  DP_INFO ("Process manager successfully de-initialized");

  return DP_OK;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPByte
dpIsInitted (void)
{
  return init;
}
