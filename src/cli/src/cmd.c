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
#include "logger.h"
#include "msg.h"
#include "plat.h"
#include "util.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define WAIT_ATTEMPTS (3)
#define SLOWDOWN_INTERVAL (1)
#define CLEAN_EXIT_WITH_FAIL(id)                                              \
  {                                                                           \
    CHECK_POSIX (dpcMsgRm ((id)), "Failed to remove message queue",           \
                 EXIT_WITH_FAILURE);                                          \
    DPC_DSLOW ("Removed msg id: %d", (id));                                   \
    EXIT_WITH_FAILURE                                                         \
  }

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static const struct DPCOption cmdopts[]
    = { { "dir", DPCRequiredArgument, NULL, 'D' },
        { "start", DPCNoArgument, NULL, 's' },
        { "restart", DPCNoArgument, NULL, 'r' },
        { "stop", DPCNoArgument, NULL, 't' },
        { "list", DPCNoArgument, NULL, 'l' },
        { "create", DPCNoArgument, NULL, 'c' },
        { "delete", DPCNoArgument, NULL, 'd' },
        { "persist", DPCNoArgument, NULL, 'p' },
        { "import", DPCRequiredArgument, NULL, 'i' },
        { "export", DPCRequiredArgument, NULL, 'e' },
        { "kill", DPCNoArgument, NULL, 'k' },
        { "all", DPCNoArgument, NULL, 'a' },
        { "force", DPCNoArgument, NULL, 'f' },
        { NULL, 0, NULL, 0 } };

static void
dpcCmdReplyWait (DPInt recvid)
{
  DPDword attempts = 0;
  while (TRUE)
    {
      if (attempts >= WAIT_ATTEMPTS)
        {
          DPC_WARN ("Reply timeout");
          break;
        }

      // Try to read from queue.
      DPCMsgAny reply = { .cmd = DPC_CNONE, .data.text = { 0 } };
      DPInt recv = dpcMsgRcv (recvid, &reply, SIZEOF (reply.data), 0);
      CHECK_POSIX (recv, "Failed to receive message",
                   CLEAN_EXIT_WITH_FAIL (recvid));

      if (recv == 0 && reply.cmd == DPC_CNONE)
        {
          // Throttle the loop.
          dpcSleep (SLOWDOWN_INTERVAL);

          ++attempts;
          continue;
        }

      DPC_DSLOW ("Received reply: %s", dpcCommandStr (reply.cmd));

      if (reply.cmd == DPC_CREPLY)
        {
          DPCMsgProcInfo *info = &reply.data.pinfo;
          DPC_INFO ("%4d: %-16s %-8s [ %6d ] %8u ↻", info->id, info->name,
                    info->state == DP_STOPPED ? STATE_STOPPED_STR
                                              : STATE_RUNNING_STR,
                    info->pid, info->restarts);
          continue;
        }

      if (reply.cmd == DPC_CERROR)
        {
          DPC_ERROR ("%s", reply.data.text);
          continue;
        }

      if (reply.cmd == DPC_CFINISH)
        {
          break;
        }

      DPC_FATAL ("Unexpected reply");
      CLEAN_EXIT_WITH_FAIL (recvid)
    }
}

static DPSize
dpcSerializeArgsSize (DPInt argc, DPChar *argv[])
{
  DPSize result = 0;

  // First, store argc.
  result += SIZEOF (DPInt);

  // Then store each argument with its length.
  for (DPInt i = 0; i < argc; ++i)
    {
      // Length goes first.
      result += SIZEOF (DPSize);

      // +1 for NULL terminator.
      DPSize sz = dpcStrlen (argv[i]) + 1;
      result += sz;
    }

  return result;
}

static void
dpcSerializeArgs (DPChar *buff, DPSize size, DPInt argc, DPChar *argv[])
{
  DPChar *ptr = buff;
  DPSize remaining = size;

  // First, store argc.
  if (remaining < SIZEOF (DPInt))
    {
      return;
    }
  dpcMemcpy (ptr, &argc, SIZEOF (DPInt));
  ptr += SIZEOF (DPInt);
  remaining -= SIZEOF (DPInt);

  // Then store each argument with its length.
  for (DPInt i = 0; i < argc; ++i)
    {
      // +1 for NULL terminator.
      DPSize sz = dpcStrlen (argv[i]) + 1;

      // Store length.
      if (remaining < SIZEOF (DPSize))
        {
          return;
        }
      dpcMemcpy (ptr, &sz, SIZEOF (DPSize));
      ptr += SIZEOF (DPSize);
      remaining -= SIZEOF (DPSize);

      // Store string.
      if (remaining < sz)
        {
          return;
        }
      dpcMemcpy (ptr, argv[i], sz);
      ptr += sz;
      remaining -= sz;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
dpcCmd (DPInt argc, DPChar *argv[])
{
  DPChar workdir[WORKDIR_MAX] = { 0 };
  DPCMsgText req = { .cmd = DPC_CNONE, .text = { 0 } };
  DPChar *arg = req.text;
  DPBool applyall = FALSE;
  DPBool force = FALSE;

  // Read command line options.
  DPInt c;
  while ((c = dpcGetoptLong (argc, argv, "D:srtlcdpi:e:kaf", cmdopts, NULL))
         != -1)
    {
      if (req.cmd != DPC_CNONE && c != 'D' && c != 'a' && c != 'f')
        {
          // Only one command is allowed ...
          DPC_ERROR ("Too many commands");

          dpcPrintUsage (argv[0]);

          EXIT_WITH_FAILURE
        }

      DPChar *optarg = NULL;
      switch (c)
        {
        case 'D':
          optarg = dpcOptArg ();
          DPC_DSLOW ("Arg: --dir=%s", optarg);
          dpcStrncpy (workdir, optarg, CHARSMAX (workdir));
          break;

        case 's':
          req.cmd = DPC_CSTART;
          DPC_DSLOW ("Arg: --start");
          break;

        case 'r':
          req.cmd = DPC_CRESTART;
          DPC_DSLOW ("Arg: --restart");
          break;

        case 't':
          req.cmd = DPC_CSTOP;
          DPC_DSLOW ("Arg: --stop");
          break;

        case 'l':
          req.cmd = DPC_CLIST;
          DPC_DSLOW ("Arg: --list");
          break;

        case 'c':
          req.cmd = DPC_CCREATE;
          DPC_DSLOW ("Arg: --create");
          break;

        case 'd':
          req.cmd = DPC_CDELETE;
          DPC_DSLOW ("Arg: --delete");
          break;

        case 'p':
          req.cmd = DPC_CPERSIST;
          DPC_DSLOW ("Arg: --persist");
          break;

        case 'i':
          {
            req.cmd = DPC_CIMPORT;
            optarg = dpcOptArg ();
            DPC_DSLOW ("Arg: --import=%s", optarg);
            DPChar *path = dpcRealpath (optarg, NULL);
            if (path == NULL)
              {
                DPC_ERROR ("Imported file does not exist");

                EXIT_WITH_FAILURE
              }
            dpcStrncpy (arg, path, CHARSMAX (req.text));
            dpcFree (path);
            break;
          }

        case 'e':
          {
            req.cmd = DPC_CEXPORT;
            optarg = dpcOptArg ();
            DPC_DSLOW ("Arg: --export=%s", optarg);
            DPChar *path = dpcRealpathne (optarg);
            if (path == NULL)
              {
                DPC_ERROR ("Invalid export filepath");

                EXIT_WITH_FAILURE
              }
            dpcStrncpy (arg, path, CHARSMAX (req.text));
            dpcFree (path);
            break;
          }

        case 'a':
          DPC_DSLOW ("Arg: --all");
          applyall = TRUE;
          dpcStrncpy (arg, "*", CHARSMAX (req.text));
          break;

        case 'f':
          DPC_DSLOW ("Arg: --force");
          force = TRUE;
          break;

        case 'k':
          DPC_DSLOW ("Arg: --kill");
          req.cmd = DPC_CKILL;
          break;

        default:
          DPC_ERROR ("Unknown argument(s)");

          dpcPrintUsage (argv[0]);

          EXIT_WITH_FAILURE
        }

      if (req.cmd == DPC_CCREATE)
        {
          break;
        }
    }

  if (req.cmd == DPC_CNONE)
    {
      // No command present ...
      DPC_ERROR ("Missing command");

      dpcPrintUsage (argv[0]);

      EXIT_WITH_FAILURE
    }

  DPInt idx = dpcOptInd ();
  if (req.cmd == DPC_CIMPORT || req.cmd == DPC_CEXPORT
      || req.cmd == DPC_CPERSIST || req.cmd == DPC_CKILL || applyall)
    {
      // Options --import=..., --export=..., and --all expect no additional
      // args.
      if (idx < argc)
        {
          DPC_ERROR ("Unexpected processes");

          dpcPrintUsage (argv[0]);

          EXIT_WITH_FAILURE
        }
    }
  else if (req.cmd == DPC_CCREATE)
    {
      // Create command expects name of the app plus its args.
      if (idx >= argc)
        {
          DPC_ERROR ("Missing process definition");

          dpcPrintUsage (argv[0]);

          EXIT_WITH_FAILURE
        }

      // Check serialized arg size.
      const DPSize sz = dpcSerializeArgsSize (argc - idx, &argv[idx]);
      if (sz > SIZEOF (req.text))
        {
          DPC_ERROR ("Process definition is too long");

          dpcPrintUsage (argv[0]);

          EXIT_WITH_FAILURE
        }
    }
  else
    {
      // Other commands expects at least one target.
      if (idx >= argc)
        {
          DPC_ERROR ("No processes specified");

          dpcPrintUsage (argv[0]);

          EXIT_WITH_FAILURE
        }

      // Pre-validate argument lengths.
      DPInt i = idx;
      while (i < argc)
        {
          if (dpcStrlen (argv[i]) > CHARSMAX (req.text))
            {
              DPC_ERROR ("Process name too long");

              dpcPrintUsage (argv[0]);

              EXIT_WITH_FAILURE
            }
          ++i;
        }
    }

  // Determine and eventually create work directory.
  dpcPrepareWorkdir (workdir, SIZEOF (workdir));

  DPInt recvkey = dpcFtok (workdir, DPC_DS2C);
  CHECK_POSIX (recvkey, "Failed to obtain ingress message queue key",
               EXIT_WITH_FAILURE);
  DPC_DSLOW ("Recv msg key: %d", recvkey);

  DPInt recvid = dpcMsgGet (recvkey, TRUE, !force);
  CHECK_POSIX (recvid, "Another Daemon Pal command is being processed",
               EXIT_WITH_FAILURE);
  DPC_DSLOW ("Recv msg id: %d", recvid);

  if (force)
    {
      // Remove queue to ensure removal of any hanging messages.
      CHECK_POSIX (dpcMsgRm ((recvid)), "Failed to remove message queue",
                   EXIT_WITH_FAILURE);
      DPC_DSLOW ("Removed msg id: %d", (recvid));

      // Get the queue again.
      recvid = dpcMsgGet (recvkey, TRUE, !force);
      CHECK_POSIX (recvid, "Another Daemon Pal command is being processed",
                   EXIT_WITH_FAILURE);
      DPC_DSLOW ("Recv msg id: %d", recvid);
    }

  DPInt sendkey = dpcFtok (workdir, DPC_DC2S);
  CHECK_POSIX (sendkey, "Failed to obtain egress message queue key",
               CLEAN_EXIT_WITH_FAIL (recvid));
  DPC_DSLOW ("Send msg key: %d", sendkey);

  DPInt sendid = dpcMsgGet (sendkey, FALSE, FALSE);
  CHECK_POSIX (sendid, "No Daemon Pal control process is running",
               CLEAN_EXIT_WITH_FAIL (recvid));
  DPC_DSLOW ("Send msg id: %d", sendid);

  if (req.cmd == DPC_CIMPORT || req.cmd == DPC_CEXPORT
      || req.cmd == DPC_CPERSIST || req.cmd == DPC_CKILL || applyall)
    {
      switch (req.cmd)
        {
        case DPC_CIMPORT:
          DPC_INFO ("Importing from file: %s", arg);
          break;

        case DPC_CEXPORT:
          DPC_INFO ("Exporting to file: %s", arg);
          break;

        case DPC_CPERSIST:
          DPC_INFO ("Persisting current processes", arg);
          break;

        case DPC_CKILL:
          DPC_INFO ("Shutting down control process", arg);
          break;

        default:
          // Command is applied to all processes.
          DPC_INFO ("%s all processes", dpcCommandStr (req.cmd));
          break;
        }

      CHECK_POSIX (dpcMsgSnd (sendid, &req, SIZEOF (req.text)),
                   "Failed to send message", CLEAN_EXIT_WITH_FAIL (recvid));

      dpcCmdReplyWait (recvid);
    }
  else if (req.cmd == DPC_CCREATE)
    {
      // Length has been already validated.
      dpcSerializeArgs (arg, SIZEOF (req.text), argc - idx, &argv[idx]);

      DPC_INFO ("%s process", dpcCommandStr (req.cmd));

      CHECK_POSIX (dpcMsgSnd (sendid, &req, SIZEOF (req.text)),
                   "Failed to send message", CLEAN_EXIT_WITH_FAIL (recvid));

      dpcCmdReplyWait (recvid);
    }
  else
    {
      while (idx < argc)
        {
          // Length has been already validated.
          dpcStrncpy (arg, argv[idx], CHARSMAX (req.text));

          DPC_INFO ("%s process: %s", dpcCommandStr (req.cmd), arg);

          CHECK_POSIX (dpcMsgSnd (sendid, &req, SIZEOF (req.text)),
                       "Failed to send message",
                       CLEAN_EXIT_WITH_FAIL (recvid));

          dpcCmdReplyWait (recvid);

          ++idx;
        }
    }

  // Remove message queue.
  CHECK_POSIX (dpcMsgRm ((recvid)), "Failed to remove message queue",
               EXIT_WITH_FAILURE);
  DPC_DSLOW ("Removed id: %d", (recvid));

  // Do not return. Always exit!
  EXIT_WITH_SUCCESS

  return;
}
