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
#include "msg.h"
#include "plat.h"
#include "tail.h"
#include "util.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define WAIT_ATTEMPTS (3)
#define SLOWDOWN_INTERVAL (1)
#define LINES_DEFAULT (10)
#define LINES_DIGITS_MAX (6)
#define NO_FOLLOW_OPT (0x100)
#define CLEAN_EXIT_WITH_FAIL(id)                                              \
  {                                                                           \
    CHECK_POSIX (dpcMsgRm ((id)), "Failed to remove message queue",           \
                 EXIT_WITH_FAILURE);                                          \
    DPC_DSLOW ("Removed msg id: %d", (id));                                   \
    EXIT_WITH_FAILURE                                                         \
  }

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef DPInt (*DPCInfoHandler) (const DPCMsgProcInfo *info, void *ctx);

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
        { "log", DPCNoArgument, NULL, 'L' },
        { "lines", DPCRequiredArgument, NULL, 'n' },
        { "no-follow", DPCNoArgument, NULL, NO_FOLLOW_OPT },
        { "all", DPCNoArgument, NULL, 'a' },
        { "force", DPCNoArgument, NULL, 'f' },
        { "format", DPCRequiredArgument, NULL, 'F' },
        { NULL, 0, NULL, 0 } };

static DPInt
dpcCmdLogAdd (const DPCMsgProcInfo *info, void *ctx)
{
  DPChar name[CMD_NAME_MAX] = { 0 };
  dpcStrncpy (name, info->name, CHARSMAX (name));

  return dpcTailAdd ((DPCTail *)ctx, name);
}

static DPCStatus
dpcCmdReplyWait (DPInt recvid, DPCInfoHandler handler, void *ctx)
{
  CHECK_GENERIC (handler != NULL, "Missing reply handler",
                 RETURN_WITH_FAILURE);

  DPCStatus status = DPC_OK;
  DPDword attempts = 0;
  while (TRUE)
    {
      if (attempts >= WAIT_ATTEMPTS)
        {
          DPC_WARN ("Reply timeout");
          status = DPC_EFAIL;
          break;
        }

      // Try to read from queue.
      DPCMsgAny reply = { .cmd = DPC_CNONE, .data.text = { 0 } };
      DPInt recv = dpcMsgRcv (recvid, &reply, SIZEOF (reply.data), 0);
      CHECK_POSIX (recv, "Failed to receive message",
                   CLEAN_EXIT_WITH_FAIL (recvid));
      if (recv == 0)
        {
          // Throttle the loop.
          dpcSleep (SLOWDOWN_INTERVAL);

          ++attempts;
          continue;
        }

      DPC_DSLOW ("Received reply: %s", dpcCommandStr (reply.cmd));

      if (reply.cmd == DPC_CREPLY)
        {
          if (handler (&reply.data.pinfo, ctx) < 0)
            {
              status = DPC_EFAIL;
            }
          continue;
        }

      if (reply.cmd == DPC_CERROR)
        {
          DPC_ERROR ("%s", reply.data.text);
          status = DPC_EFAIL;
          continue;
        }

      if (reply.cmd == DPC_CFINISH)
        {
          break;
        }

      DPC_FATAL ("Unexpected reply");
      CLEAN_EXIT_WITH_FAIL (recvid)
    }

  return status;
}

static DPBool
dpcIsCount (const DPChar *num)
{
  DPSize len = dpcStrnlen (num, LINES_DIGITS_MAX + 1);
  if (len == 0 || len > LINES_DIGITS_MAX)
    {
      return FALSE;
    }

  for (DPSize i = 0; i < len; ++i)
    {
      if (num[i] < '0' || num[i] > '9')
        {
          return FALSE;
        }
    }

  return TRUE;
}

static void
dpcCmdLog (DPCTail *tail, DPSize lines, DPBool follow)
{
  DPInt result = dpcTailStart (tail, lines);
  if (result == DPC_OK)
    {
      result = dpcTailPoll (tail);
    }

  while (result == DPC_OK && follow)
    {
      dpcUSleep (TAIL_POLL_INTERVAL);

      result = dpcTailPoll (tail);
    }

  if (result == DPC_OK)
    {
      result = dpcTailFlush (tail);
    }

  dpcTailDestroy (tail);

  if (result != DPC_OK)
    {
      EXIT_WITH_FAILURE
    }

  EXIT_WITH_SUCCESS
}

static DPSize
dpcSerializeArgsSize (const DPChar *dir, DPInt argc, DPChar *argv[])
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

  // Finally, store the process directory with its length.
  result += SIZEOF (DPSize) + dpcStrlen (dir) + 1;

  return result;
}

static void
dpcSerializeArgs (DPChar *buff, DPSize size, const DPChar *dir, DPInt argc,
                  DPChar *argv[])
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

  // Finally, store the process directory with its length.
  DPSize sz = dpcStrlen (dir) + 1;
  if (remaining < SIZEOF (DPSize) + sz)
    {
      return;
    }
  dpcMemcpy (ptr, &sz, SIZEOF (DPSize));
  ptr += SIZEOF (DPSize);
  dpcMemcpy (ptr, dir, sz);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
dpcCmd (DPInt argc, DPChar *argv[])
{
  DPChar workdir[WORKDIR_MAX] = { 0 };
  DPChar cwd[WORKDIR_MAX] = { 0 };
  DPCMsgText req = { .cmd = DPC_CNONE, .text = { 0 } };
  DPChar *arg = req.text;
  DPBool applyall = FALSE;
  DPBool force = FALSE;
  DPSize lines = LINES_DEFAULT;
  DPBool linesSet = FALSE;
  DPBool follow = TRUE;
  DPCTail tail = NULL;
  DPCList list = { .fmt = DPC_FPLAIN, .count = 0 };
  DPBool fmtSet = FALSE;
  DPCStatus status = DPC_OK;

  // Read command line options.
  DPInt c;
  while (
      (c = dpcGetoptLong (argc, argv, "D:srtlcdpi:e:kLn:afF:", cmdopts, NULL))
      != -1)
    {
      if (req.cmd != DPC_CNONE && c != 'D' && c != 'a' && c != 'f' && c != 'n'
          && c != 'F' && c != NO_FOLLOW_OPT)
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

        case 'L':
          DPC_DSLOW ("Arg: --log");
          req.cmd = DPC_CLOG;
          break;

        case 'n':
          optarg = dpcOptArg ();
          DPC_DSLOW ("Arg: --lines=%s", optarg);
          if (!dpcIsCount (optarg))
            {
              DPC_ERROR ("Invalid number of lines");

              dpcPrintUsage (argv[0]);

              EXIT_WITH_FAILURE
            }
          lines = dpcAtoi (optarg);
          linesSet = TRUE;
          break;

        case NO_FOLLOW_OPT:
          DPC_DSLOW ("Arg: --no-follow");
          follow = FALSE;
          break;

        case 'F':
          optarg = dpcOptArg ();
          DPC_DSLOW ("Arg: --format=%s", optarg);
          if (dpcListParseFmt (&list.fmt, optarg) != DPC_OK)
            {
              DPC_ERROR ("Invalid output format");

              dpcPrintUsage (argv[0]);

              EXIT_WITH_FAILURE
            }
          fmtSet = TRUE;
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

  if (req.cmd != DPC_CLOG && (linesSet || !follow))
    {
      DPC_ERROR ("Options --lines and --no-follow require --log");

      dpcPrintUsage (argv[0]);

      EXIT_WITH_FAILURE
    }

  if (req.cmd != DPC_CLIST && fmtSet)
    {
      DPC_ERROR ("Option --format requires --list");

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

      // The created process runs in the current directory of the caller.
      if (dpcGetcwd (cwd, SIZEOF (cwd)) == NULL)
        {
          DPC_ERROR ("Cannot determine current directory");

          EXIT_WITH_FAILURE
        }

      // Check serialized arg size.
      const DPSize sz = dpcSerializeArgsSize (cwd, argc - idx, &argv[idx]);
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

  // The log command only resolves its targets via the list command. Its log
  // files are followed directly once the reply queue is released.
  const DPBool logmode = req.cmd == DPC_CLOG;
  DPCInfoHandler handler = dpcListRow;
  void *ctx = &list;
  if (logmode)
    {
      req.cmd = DPC_CLIST;
      handler = dpcCmdLogAdd;
      ctx = &tail;

      if (dpcTailInit (&tail, workdir, dpcStdout ()) < 0)
        {
          CLEAN_EXIT_WITH_FAIL (recvid)
        }
    }

  if (req.cmd == DPC_CIMPORT || req.cmd == DPC_CEXPORT
      || req.cmd == DPC_CPERSIST || req.cmd == DPC_CKILL || applyall)
    {
      switch (logmode ? DPC_CLOG : req.cmd)
        {
        case DPC_CLOG:
          // Keep the output clean for the log lines.
          break;

        case DPC_CLIST:
          // Keep the output clean for the machine-readable formats.
          if (list.fmt == DPC_FPLAIN)
            {
              DPC_INFO ("%s all processes", dpcCommandStr (req.cmd));
            }
          break;

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

      if (!logmode)
        {
          dpcListHeader (&list);
        }

      CHECK_POSIX (dpcMsgSnd (sendid, &req, SIZEOF (req.text)),
                   "Failed to send message", CLEAN_EXIT_WITH_FAIL (recvid));

      status = dpcCmdReplyWait (recvid, handler, ctx);
    }
  else if (req.cmd == DPC_CCREATE)
    {
      // Length has been already validated.
      dpcSerializeArgs (arg, SIZEOF (req.text), cwd, argc - idx, &argv[idx]);

      DPC_INFO ("%s process", dpcCommandStr (req.cmd));

      CHECK_POSIX (dpcMsgSnd (sendid, &req, SIZEOF (req.text)),
                   "Failed to send message", CLEAN_EXIT_WITH_FAIL (recvid));

      status = dpcCmdReplyWait (recvid, handler, ctx);
    }
  else
    {
      if (!logmode)
        {
          dpcListHeader (&list);
        }

      while (idx < argc)
        {
          // Length has been already validated.
          dpcStrncpy (arg, argv[idx], CHARSMAX (req.text));

          if (!logmode && list.fmt == DPC_FPLAIN)
            {
              DPC_INFO ("%s process: %s", dpcCommandStr (req.cmd), arg);
            }

          CHECK_POSIX (dpcMsgSnd (sendid, &req, SIZEOF (req.text)),
                       "Failed to send message",
                       CLEAN_EXIT_WITH_FAIL (recvid));

          // Keep processing remaining targets, but remember the failure.
          if (dpcCmdReplyWait (recvid, handler, ctx) != DPC_OK)
            {
              status = DPC_EFAIL;
            }

          ++idx;
        }
    }

  // Close the list even on failure so the output stays well-formed.
  if (!logmode)
    {
      dpcListFooter (&list);
    }

  // Remove message queue.
  CHECK_POSIX (dpcMsgRm ((recvid)), "Failed to remove message queue",
               EXIT_WITH_FAILURE);
  DPC_DSLOW ("Removed id: %d", (recvid));

  if (status != DPC_OK)
    {
      // The control process reported an error (or did not reply).
      dpcTailDestroy (&tail);

      EXIT_WITH_FAILURE
    }

  if (logmode)
    {
      // Does not return.
      dpcCmdLog (&tail, lines, follow);
    }

  // Do not return. Always exit!
  EXIT_WITH_SUCCESS

  return;
}
