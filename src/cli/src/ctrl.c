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
#include "msg.h"
#include "plat.h"
#include "util.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define WORKDIR_MAX (1024)
#define INI_MAX (1024)
#define SHUTDOWN_ATTEMPTS (16)
#define SLOWDOWN_INTERVAL (1)
#define ARG_MAX (32)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static const DPChar DPAL_INI[] = "dpal.ini";

static const struct DPCOption ctrlopts[]
    = { { "control", DPCNoArgument, NULL, 'C' },
        { "dir", DPCRequiredArgument, NULL, 'D' },
        { "auto-start", DPCNoArgument, NULL, 'a' },
        { "ini", DPCRequiredArgument, NULL, 'i' },
        { "force", DPCNoArgument, NULL, 'f' },
        { NULL, 0, NULL, 0 } };

static volatile DPBool shutdown = FALSE;

// ===============================================================
// Shutdown signal
// ===============================================================

static void
dpcShutdown (void)
{
  shutdown = TRUE;
}

static DPBool
dpcIsShutdown (void)
{
  return shutdown;
}

static void
handleInt (DPInt val)
{
  UNUSED (val);

  dpcShutdown ();
}

static DPInt
dpcSetSig (void)
{
  CHECK_POSIX (dpcSigIntHandler (handleInt), "Cannot set SIGINT handler",
               RETURN_WITH_FAILURE);

  CHECK_POSIX (dpcSigTermHandler (handleInt), "Cannot set SIGTERM handler",
               RETURN_WITH_FAILURE);

  return DPC_OK;
}

static DPInt
dpcUnsetSig (void)
{
  CHECK_POSIX (dpcSigIntHandler (NULL), "Cannot unset SIGINT handler",
               RETURN_WITH_FAILURE);

  CHECK_POSIX (dpcSigIntHandler (NULL), "Cannot unset SIGTERM handler",
               RETURN_WITH_FAILURE);

  return DPC_OK;
}

// ===============================================================
// Before exec callback
// ===============================================================

static void
dpcExecBefore (DPId id)
{
  UNUSED (id);

  dpcUnsetSig ();
}

// ===============================================================
// Message queue
// ===============================================================

static DPInt
dpcMsgDestroy (DPInt msgid)
{
  CHECK_POSIX (dpcMsgRm (msgid), "Failed to remove message queue",
               RETURN_WITH_FAILURE);
  DPC_DSLOW ("Removed msg id: %d", msgid);

  return DPC_OK;
}

static DPInt
dpcMsgInitRecv (const DPChar *workdir, DPBool force)
{
  DPInt recvkey = dpcFtok (workdir, DPC_DC2S);
  CHECK_POSIX (recvkey, "Failed to obtain ingress message queue key",
               RETURN_WITH_FAILURE);
  DPC_DSLOW ("Recv msg key: %d", recvkey);

  DPInt recvid = dpcMsgGet (recvkey, TRUE, !force);
  CHECK_POSIX (recvid, "Another Daemon Pal control process already runs",
               RETURN_WITH_FAILURE);
  DPC_DSLOW ("Recv msg id: %d", recvid);

  if (force)
    {
      // Remove queue to ensure removal of any hanging messages.
      dpcMsgDestroy (recvid);

      // Get the queue again.
      recvid = dpcMsgGet (recvkey, TRUE, TRUE);
      CHECK_POSIX (recvid, "Another Daemon Pal control process already runs",
                   RETURN_WITH_FAILURE);
      DPC_DSLOW ("Recv msg id: %d", recvid);
    }

  return recvid;
}

static DPInt
dpcMsgInitSend (const DPChar *workdir)
{
  DPInt sendkey = dpcFtok (workdir, DPC_DS2C);
  CHECK_POSIX (sendkey, "Failed to obtain reply message queue key",
               RETURN_WITH_FAILURE);
  DPC_DSLOW ("Send msg key: %d", sendkey);

  DPInt sendid = dpcMsgGet (sendkey, FALSE, FALSE);
  CHECK_POSIX (sendid,
               "Cannot reply! No Daemon Pal command process is running",
               RETURN_WITH_FAILURE);
  DPC_DSLOW ("Send msg id: %d", sendid);

  return sendid;
}

static DPInt
dpcExecReplyFinish (DPInt sendid)
{
  DPCMsgEmpty msg = { .cmd = DPC_CFINISH };
  CHECK_POSIX (dpcMsgSnd (sendid, &msg, 0), "Failed to send finish message",
               RETURN_WITH_FAILURE);

  return DPC_OK;
}

static DPInt
dpcExecReplyError (DPInt sendid, const DPChar *msg)
{
  DPCMsgText err = { .cmd = DPC_CERROR, .text = { 0 } };
  dpcStrncpy (err.text, msg, CHARSMAX (err.text));

  CHECK_POSIX (dpcMsgSnd (sendid, &err, SIZEOF (err.text)),
               "Failed to send error message", RETURN_WITH_FAILURE);

  return DPC_OK;
}

static DPInt
dpcExecProcFind (DPInt sendid, const DPChar *arg, DPId *id)
{
  if (dpProcFind (id, arg) == DP_OK)
    {
      return DPC_OK;
    }

  const DPChar *ptr = arg;
  DPBool posnum = TRUE;
  while (*ptr != '\0')
    {
      if (*ptr < '0' || *ptr > '9')
        {
          posnum = FALSE;
          break;
        }

      ++ptr;
    }

  if (posnum)
    {
      DPId proc = dpcAtoi (arg);
      if (dpProcExists (proc) == DP_OK)
        {
          *id = proc;

          return DPC_OK;
        }
    }

  DPC_WARN ("Process not found: %s", arg);
  dpcExecReplyError (sendid, "Process not found");

  return DPC_EFAIL;
}

// ===============================================================
// Main control loop
// ===============================================================

static DPInt
dpcDpalInit (const DPChar *ini, DPBool initialize)
{
  CHECK_DPAL (dpInit (), "Cannot initialize dpal library",
              RETURN_WITH_FAILURE);

  CHECK_DPAL (dpExecBefore (dpcExecBefore), "Cannot setup before exec handler",
              RETURN_WITH_FAILURE);

  if (initialize && !dpcFileExists (ini))
    {
      DPC_DEBUG ("Creating default empty INI file: %s", ini);

      CHECK_DPAL (dpProcSave (ini), "Cannot create default INI file",
                  RETURN_WITH_FAILURE);
    }

  CHECK_DPAL (dpProcLoad (ini), "Cannot load processes from INI file",
              RETURN_WITH_FAILURE);

  return DPC_OK;
}

static DPInt
dpcDpalDestroy (void)
{
  CHECK_DPAL (dpDestroy (), "Cannot de-initialize dpal library",
              RETURN_WITH_FAILURE);

  return DPC_OK;
}

static DPInt
dpcProcStart (DPInt sendid, DPId id)
{
  const DPChar *ERROR = "Cannot start process";

  CHECK_DPAL (dpProcStart (id), ERROR, {
    dpcExecReplyError (sendid, ERROR);
    return DPC_EFAIL;
  });

  return DPC_OK;
}

static DPInt
dpcProcStartAll (DPInt sendid)
{
  DPProcIter it;
  CHECK_DPAL (dpProcIter (&it), "Cannot get process iterator",
              EXIT_WITH_FAILURE);

  DPId next;
  DPInt result = DPC_OK;
  while (dpProcIterNext (&it, &next) != DP_BREAK)
    {
      if (dpcProcStart (sendid, next) < 0)
        {
          result = DPC_EFAIL;
        }
    }

  return result;
}

static DPInt
dpcProcStop (DPInt sendid, DPId id)
{
  const DPChar *ERROR = "Cannot stop process";

  CHECK_DPAL (dpProcStop (id), ERROR, {
    dpcExecReplyError (sendid, ERROR);
    return DPC_EFAIL;
  });

  return DPC_OK;
}

static DPInt
dpcProcStopAll (DPInt sendid)
{
  DPProcIter it;
  CHECK_DPAL (dpProcIter (&it), "Cannot get process iterator",
              EXIT_WITH_FAILURE);

  DPId next;
  DPInt result = DPC_OK;
  while (dpProcIterNext (&it, &next) != DP_BREAK)
    {
      if (dpcProcStop (sendid, next) < 0)
        {
          result = DPC_EFAIL;
        }
    }

  return result;
}

static DPInt
dpcProcRestart (DPInt sendid, DPId id)
{
  const DPChar *ERROR = "Cannot restart process";

  CHECK_DPAL (dpProcRestart (id), ERROR, {
    dpcExecReplyError (sendid, ERROR);
    return DPC_EFAIL;
  });

  return DPC_OK;
}

static DPInt
dpcProcRestartAll (DPInt sendid)
{
  DPProcIter it;
  CHECK_DPAL (dpProcIter (&it), "Cannot get process iterator",
              EXIT_WITH_FAILURE);

  DPId next;
  DPInt result = DPC_OK;
  while (dpProcIterNext (&it, &next) != DP_BREAK)
    {
      if (dpcProcRestart (sendid, next) < 0)
        {
          result = DPC_EFAIL;
        }
    }

  return result;
}

static DPInt
dpcProcUnreg (DPInt sendid, DPId id)
{
  const DPChar *ERROR = "Cannot remove process";

  CHECK_DPAL (dpProcUnreg (id), ERROR, {
    dpcExecReplyError (sendid, ERROR);
    return DPC_EFAIL;
  });

  return DPC_OK;
}

static DPInt
dpcProcUnregAll (DPInt sendid)
{
  DPProcIter it;
  CHECK_DPAL (dpProcIter (&it), "Cannot get process iterator",
              EXIT_WITH_FAILURE);

  DPId next;
  DPInt result = DPC_OK;
  while (dpProcIterNext (&it, &next) != DP_BREAK)
    {
      if (dpcProcUnreg (sendid, next) < 0)
        {
          result = DPC_EFAIL;
        }
    }

  return result;
}

static DPInt
dpcExecStart (DPInt sendid, const DPChar *arg)
{
  if (dpcStrcmp (arg, "*") == 0)
    {
      return dpcProcStartAll (sendid);
    }

  DPId id;
  if (dpcExecProcFind (sendid, arg, &id) < 0)
    {
      return DPC_EFAIL;
    }

  if (dpcProcStart (sendid, id) < 0)
    {
      return DPC_EFAIL;
    }

  return DPC_OK;
}

static DPInt
dpcExecStop (DPInt sendid, const DPChar *arg)
{
  if (dpcStrcmp (arg, "*") == 0)
    {
      return dpcProcStopAll (sendid);
    }

  DPId id;
  if (dpcExecProcFind (sendid, arg, &id) < 0)
    {
      return DPC_EFAIL;
    }

  if (dpcProcStop (sendid, id) < 0)
    {
      return DPC_EFAIL;
    }

  return DPC_OK;
}

static DPInt
dpcExecRestart (DPInt sendid, const DPChar *arg)
{
  if (dpcStrcmp (arg, "*") == 0)
    {
      return dpcProcRestartAll (sendid);
    }

  DPId id;
  if (dpcExecProcFind (sendid, arg, &id) < 0)
    {
      return DPC_EFAIL;
    }

  if (dpcProcRestart (sendid, id) < 0)
    {
      return DPC_EFAIL;
    }

  return DPC_OK;
}

static void
dpcSanitizeName (DPChar *name)
{
  if (name == NULL || name[0] == '\0')
    {
      return;
    }

  // First character must be a letter.
  if (!((name[0] >= 'A' && name[0] <= 'Z')
        || (name[0] >= 'a' && name[0] <= 'z')))
    {
      // If first char is not a letter, change it to 'p'.
      name[0] = 'p';
    }

  // Check and fix remaining characters.
  DPSize i = 1;
  while (name[i] != '\0')
    {
      DPChar c = name[i];

      // Check if character is valid.
      if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9') || (c == '_' || c == '-')))
        {
          // Replace invalid character with underscore.
          name[i] = '_';
        }
      i++;
    }
}

static DPInt
dpcExecCreate (DPInt sendid, const DPChar *arg)
{
  const DPChar *ptr = arg;
  DPInt argc = 0;
  const DPChar *args[ARG_MAX] = { 0 };

  // Read argc.
  dpcMemcpy (&argc, ptr, SIZEOF (DPInt));
  ptr += SIZEOF (DPInt);
  DPC_DSLOW ("Create process argc: %d", argc);
  if (argc <= 0)
    {
      DPC_WARN ("No arguments");
      dpcExecReplyError (sendid, "No arguments");
      return DPC_EFAIL;
    }
  if (argc >= ARG_MAX)
    {
      DPC_WARN ("Too many arguments: %d", argc);
      dpcExecReplyError (sendid, "Too many arguments");
      return DPC_EFAIL;
    }

  // Read each argument.
  DPSize remain = CMD_ARG_MAX - SIZEOF (DPInt);
  for (DPInt i = 0; i < argc; ++i)
    {
      DPSize sz = 0;
      if (remain < SIZEOF (sz))
        {
          DPC_WARN ("Argument too long");
          dpcExecReplyError (sendid, "Argument too long");
          return DPC_EFAIL;
        }
      dpcMemcpy (&sz, ptr, SIZEOF (sz));
      if (sz == 0)
        {
          DPC_WARN ("Argument is empty");
          dpcExecReplyError (sendid, "Argument is empty");
          return DPC_EFAIL;
        }
      if (sz > remain - SIZEOF (sz))
        {
          DPC_WARN ("Argument too long");
          dpcExecReplyError (sendid, "Argument too long");
          return DPC_EFAIL;
        }
      ptr += SIZEOF (sz);
      remain -= sz + SIZEOF (sz);

      args[i] = ptr;
      ptr += sz;
    }

  // Read the process directory (absolute path) which follows the args.
  DPSize dirsz = 0;
  if (remain >= SIZEOF (dirsz))
    {
      dpcMemcpy (&dirsz, ptr, SIZEOF (dirsz));
    }
  if (dirsz <= 1 || dirsz > remain - SIZEOF (dirsz)
      || ptr[SIZEOF (dirsz)] != '/' || ptr[SIZEOF (dirsz) + dirsz - 1] != '\0')
    {
      DPC_WARN ("Invalid process directory");
      dpcExecReplyError (sendid, "Invalid process directory");
      return DPC_EFAIL;
    }
  const DPChar *dir = ptr + SIZEOF (dirsz);

#ifdef _DEBUG
  for (DPInt i = 0; i < argc; ++i)
    {
      DPC_DSLOW ("Create process argv[%d]: %s", i, args[i]);
    }
  DPC_DSLOW ("Create process dir: %s", dir);
#endif

  DPChar *app = dpcStrndup (args[0], DP_APP_MAX);
  DPChar *appdup = dpcStrndup (app, DP_APP_MAX);
  DPChar *basename = dpcBasename (appdup);
  if (dpcStrcmp (basename, "/") == 0 || dpcStrcmp (basename, ".") == 0
      || dpcStrcmp (basename, "..") == 0)
    {
      DPC_WARN ("Process basename refers to a directory: %s", basename);

      dpcFree (appdup);
      dpcFree (app);

      dpcExecReplyError (sendid, "Process basename refers to a directory");
      return DPC_EFAIL;
    }

  DPChar suff[8] = { 0 };
  DPChar name[DP_NAME_MAX + 8] = { 0 };
  dpcStrncpy (name, basename, CHARSMAX (name));
  dpcSanitizeName (name);

  DPByte i = 1;
  while (dpProcFind (NULL, name) == DP_OK)
    {
      dpcStrncpy (name, basename, CHARSMAX (name));
      dpcSanitizeName (name);
      dpcStrncat (name, "-", 1);
      dpcItoa10 (i, suff);
      dpcStrncat (name, suff, 7);
      ++i;

      // Prevent infinite loop.
      if (i == 0)
        {
          DPC_WARN ("Cannot infer process name: %s", basename);

          dpcExecReplyError (sendid, "Cannot infer process name");
          return DPC_EFAIL;
        }
    }

  DPProcDef def;
  args[0] = basename;
  const DPChar *envs[] = { NULL };
  const DPChar *ERROR_DEF = "Cannot initialize definition";
  CHECK_DPAL (dpProcDefInit (&def, name, app, dir, args, envs), ERROR_DEF, {
    dpcFree (appdup);
    dpcFree (app);

    dpcExecReplyError (sendid, ERROR_DEF);
    return DPC_EFAIL;
  });
  dpcFree (appdup);
  dpcFree (app);

  DPProcAttr attr;
  const DPChar *ERROR_ATTR = "Cannot initialize attributes";
  CHECK_DPAL (dpProcAttrInit (&attr), ERROR_ATTR, {
    dpProcDefDestroy (&def);

    dpcExecReplyError (sendid, ERROR_ATTR);
    return DPC_EFAIL;
  });

  const DPChar *ERROR_REG = "Cannot create process";
  CHECK_DPAL (dpProcReg (NULL, &def, &attr), ERROR_REG, {
    dpProcAttrDestroy (&attr);
    dpProcDefDestroy (&def);

    dpcExecReplyError (sendid, ERROR_REG);
    return DPC_EFAIL;
  });

  dpProcAttrDestroy (&attr);
  dpProcDefDestroy (&def);

  return DPC_OK;
}

static DPInt
dpcExecDelete (DPInt sendid, const DPChar *arg)
{
  if (dpcStrcmp (arg, "*") == 0)
    {
      return dpcProcUnregAll (sendid);
    }

  DPId id;
  if (dpcExecProcFind (sendid, arg, &id) < 0)
    {
      return DPC_EFAIL;
    }

  if (dpcProcUnreg (sendid, id) < 0)
    {
      return DPC_EFAIL;
    }

  return DPC_OK;
}

static DPInt
dpcProcStatus (DPId id, DPCMsgInfo *msg)
{
  CHECK_GENERIC (msg != NULL, "Null message", RETURN_WITH_FAILURE);
  msg->cmd = DPC_CREPLY;

  DPCMsgProcInfo *minfo = &msg->pinfo;
  minfo->id = id;

  DPProcDef def;
  CHECK_DPAL (dpProcGetDef (&def, minfo->id),
              "Could not obtain process definition", RETURN_WITH_FAILURE);
  CHECK_DPAL (
      dpProcDefGetName (&def, minfo->name, CHARSMAX (minfo->name), NULL),
      "Could not obtain process name", RETURN_WITH_FAILURE);
  CHECK_DPAL (dpProcDefDestroy (&def), "Could not clean up process definition",
              RETURN_WITH_FAILURE);

  DPProcInfo pinfo;
  CHECK_DPAL (dpProcGetInfo (&pinfo, minfo->id),
              "Could not obtain process info", RETURN_WITH_FAILURE);
  CHECK_DPAL (dpProcInfoGetRestarts (&pinfo, &minfo->restarts),
              "Could not obtain process restarts", RETURN_WITH_FAILURE);
  CHECK_DPAL (dpProcInfoGetPid (&pinfo, &minfo->pid),
              "Could not obtain process pid", RETURN_WITH_FAILURE);
  CHECK_DPAL (dpProcInfoGetState (&pinfo, &minfo->state),
              "Could not obtain process state", RETURN_WITH_FAILURE);
  CHECK_DPAL (dpProcInfoDestroy (&pinfo), "Could not clean up process info",
              RETURN_WITH_FAILURE);

  DPC_DEBUG ("%4d: %-16s %-8s [ %6d ] %8u ↻", minfo->id, minfo->name,
             minfo->state == DP_STOPPED ? STATE_STOPPED_STR
                                        : STATE_RUNNING_STR,
             minfo->pid, minfo->restarts);

  return DPC_OK;
}

static DPInt
dpcExecListAll (DPInt sendid)
{
  DPProcIter it;
  CHECK_DPAL (dpProcIter (&it), "Cannot get process iterator",
              EXIT_WITH_FAILURE);

  DPId next;
  DPInt result = DPC_OK;
  while (dpProcIterNext (&it, &next) != DP_BREAK)
    {
      DPCMsgInfo msg = { .cmd = DPC_CNONE,
                         .pinfo = { .id = 0,
                                    .name = { 0 },
                                    .pid = 0,
                                    .restarts = 0,
                                    .state = DP_STOPPED } };
      if (dpcProcStatus (next, &msg) < 0)
        {
          dpcExecReplyError (sendid, "Cannot obtain process info");
          result = DPC_EFAIL;
          continue;
        }

      CHECK_POSIX (dpcMsgSnd (sendid, &msg, SIZEOF (msg.pinfo)),
                   "Failed to send reply message", { result = DPC_EFAIL; });
    }

  return result;
}

static DPInt
dpcExecList (DPInt sendid, const DPChar *arg)
{
  if (dpcStrcmp (arg, "*") == 0)
    {
      return dpcExecListAll (sendid);
    }

  DPId id;
  if (dpcExecProcFind (sendid, arg, &id) < 0)
    {
      return DPC_EFAIL;
    }

  DPCMsgInfo msg;
  if (dpcProcStatus (id, &msg) < 0)
    {
      return DPC_EFAIL;
    }

  CHECK_POSIX (dpcMsgSnd (sendid, &msg, SIZEOF (msg.pinfo)),
               "Failed to send reply message", RETURN_WITH_FAILURE);

  return DPC_OK;
}

static DPInt
dpcExecPersist (DPInt sendid)
{
  const DPChar *ERROR = "Cannot persist process ecosystem";

  CHECK_DPAL (dpProcSave (DPAL_INI), ERROR, {
    dpcExecReplyError (sendid, ERROR);
    return DPC_EFAIL;
  });

  return DPC_OK;
}

static DPInt
dpcExecImport (DPInt sendid, const DPChar *arg)
{
  const DPChar *ERROR = "Cannot import process ecosystem";

  if (arg[0] != '/')
    {
      DPC_DEBUG ("Import filepath is not absolute");
      dpcExecReplyError (sendid, ERROR);
      return DPC_EFAIL;
    }

  CHECK_DPAL (dpProcLoad (arg), ERROR, {
    dpcExecReplyError (sendid, ERROR);
    return DPC_EFAIL;
  });

  return DPC_OK;
}

static DPInt
dpcExecExport (DPInt sendid, const DPChar *arg)
{
  const DPChar *ERROR = "Cannot export process ecosystem";

  if (arg[0] != '/')
    {
      DPC_DEBUG ("Export filepath is not absolute");
      dpcExecReplyError (sendid, ERROR);
      return DPC_EFAIL;
    }

  CHECK_DPAL (dpProcSave (arg), ERROR, {
    dpcExecReplyError (sendid, ERROR);
    return DPC_EFAIL;
  });

  return DPC_OK;
}

static DPInt
dpcExecCmd (DPInt sendid, DPCCmd cmd, const DPChar *arg)
{
  switch (cmd)
    {
    case DPC_CNONE:
      return DPC_OK;

    case DPC_CSTART:
      dpcExecStart (sendid, arg);
      return DPC_OK;

    case DPC_CSTOP:
      dpcExecStop (sendid, arg);
      return DPC_OK;

    case DPC_CRESTART:
      dpcExecRestart (sendid, arg);
      return DPC_OK;

    case DPC_CPERSIST:
      dpcExecPersist (sendid);
      return DPC_OK;

    case DPC_CIMPORT:
      dpcExecImport (sendid, arg);
      return DPC_OK;

    case DPC_CEXPORT:
      dpcExecExport (sendid, arg);
      return DPC_OK;

    case DPC_CCREATE:
      dpcExecCreate (sendid, arg);
      return DPC_OK;

    case DPC_CDELETE:
      dpcExecDelete (sendid, arg);
      return DPC_OK;

    case DPC_CKILL:
      dpcShutdown ();
      return DPC_OK;

    case DPC_CLIST:
      dpcExecList (sendid, arg);
      return DPC_OK;

    default:
      return DPC_EFAIL;
    }

  return DPC_OK;
}

static DPInt
dpcCtrlAutoStart (void)
{
  DPC_DEBUG ("Auto-starting ...");

  DPProcIter it;
  CHECK_DPAL (dpProcIter (&it), "Cannot get process iterator",
              EXIT_WITH_FAILURE);

  DPId next;
  DPInt result = DPC_OK;
  while (dpProcIterNext (&it, &next) != DP_BREAK)
    {
      CHECK_DPAL (dpProcStart (next), "Cannot auto-start process", {
        dpcShutdown ();
        result = DPC_EFAIL;
      });
    }

  return result;
}

static DPInt
dpcCtrlShutdown (void)
{
  DPC_INFO ("Initiated control process shutdown");

  for (DPDword attempt = 0; attempt < SHUTDOWN_ATTEMPTS; ++attempt)
    {
      DPProcIter it;
      if (dpProcIter (&it) != DP_OK)
        {
          break;
        }

      DPBool procRunning = FALSE;
      DPId next;
      while (dpProcIterNext (&it, &next) != DP_BREAK)
        {
          DPC_DSLOW ("dpcCtrlShutdown: Terminating: %d", next);

          DPProcInfo info;
          CHECK_DPAL (dpProcGetInfo (&info, next), "Cannot get process info",
                      { continue; });

          DPProcState state;
          CHECK_DPAL (dpProcInfoGetState (&info, &state),
                      "Cannot get process state", {
                        dpProcInfoDestroy (&info);
                        continue;
                      });

          CHECK_DPAL (dpProcInfoDestroy (&info), "Cannot destroy process info",
                      { continue; });

          if (state == DP_RUNNING)
            {
              procRunning = TRUE;
              CHECK_DPAL (dpProcStop (next), "Cannot stop process",
                          { continue; });
            }
          else
            {
              CHECK_DPAL (dpProcUnreg (next), "Cannot unregister process",
                          { continue; });
            }

          CHECK_DPAL (dpTick (),
                      "Failure in the main control loop during shutdown",
                      { continue; });
        }

      if (!procRunning)
        {
          break;
        }

      DPC_DSLOW ("dpcCtrlShutdown: Waiting for all processes to stop: %d",
                 attempt);

      dpcSleep (SLOWDOWN_INTERVAL);
    }

  return DPC_OK;
}

static void
dpcCtrlLoop (const DPChar *workdir, DPInt recvid, DPBool autostart)
{
  if (!dpcIsShutdown () && autostart)
    {
      dpcCtrlAutoStart ();
    }

  while (!dpcIsShutdown ())
    {
      CHECK_DPAL (dpTick (), "Failure in the main control loop", {
        dpcShutdown ();
        continue;
      });

      // Allocating as static to avoid using too much stack.
      static DPCMsgText msg = { .cmd = DPC_CNONE, .text = { 0 } };

      // Clear last command.
      msg.cmd = DPC_CNONE;
      msg.text[0] = '\0';

      // Try to read from queue.
      DPInt recv = dpcMsgRcv (recvid, &msg, SIZEOF (msg.text), 0);
      CHECK_POSIX (recv, "Failure when reading from message queue", {
        dpcShutdown ();
        continue;
      });

      if (recv == 0)
        {
          // Throttle the loop.
          dpcSleep (SLOWDOWN_INTERVAL);
          continue;
        }

      // If there is any execution command, perform it.
      if (msg.cmd != DPC_CNONE)
        {
          DPC_INFO ("Processing command: %s", dpcCommandStr (msg.cmd));

          DPInt sendid = dpcMsgInitSend (workdir);
          if (sendid < 0)
            {
              dpcShutdown ();
              continue;
            }

          msg.text[SIZEOF (msg.text) - 1] = '\0';
          if (dpcExecCmd (sendid, msg.cmd, msg.text) < 0)
            {
              DPC_FATAL ("Unknown command");
              dpcShutdown ();
              continue;
            }

          dpcExecReplyFinish (sendid);
          continue;
        }
    }

  dpcCtrlShutdown ();
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
dpcCtrl (DPInt argc, DPChar *argv[])
{
  DPChar workdir[WORKDIR_MAX] = { 0 };
  DPBool autostart = FALSE;
  DPChar ini[INI_MAX] = { 0 };
  DPBool initialize = FALSE;
  DPBool force = FALSE;

  // Read command line options.
  DPInt c;
  while ((c = dpcGetoptLong (argc, argv, "CD:ai:f", ctrlopts, NULL)) != -1)
    {
      switch (c)
        {
        case 'C':
          DPC_DSLOW ("Arg: --control");
          break;

        case 'D':
          DPC_DSLOW ("Arg: --dir=%s", dpcOptArg ());
          dpcStrncpy (workdir, dpcOptArg (), CHARSMAX (workdir));
          break;

        case 'a':
          DPC_DSLOW ("Arg: --auto-start");
          autostart = TRUE;
          break;

        case 'i':
          DPC_DSLOW ("Arg: --ini=%s", dpcOptArg ());
          dpcStrncpy (ini, dpcOptArg (), CHARSMAX (ini));
          break;

        case 'f':
          DPC_DSLOW ("Arg: --force");
          force = TRUE;
          break;

        default:
          DPC_ERROR ("Unknown argument(s)");

          dpcPrintUsage (argv[0]);

          EXIT_WITH_FAILURE
        }
    }

  // Determine and eventually create work directory.
  dpcPrepareWorkdir (workdir, SIZEOF (workdir));

  // Determine INI file to load.
  DPSize iniLen = dpcStrnlen (ini, SIZEOF (ini));
  CHECK_GENERIC (iniLen < SIZEOF (ini), "INI file path is too long",
                 EXIT_WITH_FAILURE);
  if (iniLen == 0)
    {
      initialize = TRUE;
      dpcStrncpy (ini, DPAL_INI, CHARSMAX (ini));
    }
  else
    {
      // Resolve the INI file before switching to workdir so that relative
      // paths are relative to the caller's current directory.
      DPChar *path = dpcRealpath (ini, NULL);
      CHECK_GENERIC (path != NULL, "INI file does not exist",
                     EXIT_WITH_FAILURE);
      dpcStrncpy (ini, path, CHARSMAX (ini));
      dpcFree (path);
    }

  DPC_DSLOW ("Switching to workdir: %s", workdir);
  CHECK_POSIX (dpcChdir (workdir), "Cannot switch to work directory",
               EXIT_WITH_FAILURE);

  // Get message queue.
  DPInt recvid = dpcMsgInitRecv (workdir, force);
  if (recvid < 0)
    {
      EXIT_WITH_FAILURE
    }

  dpcPrintLogo ();

  DPC_INFO ("Spawned new Daemon Pal control process ...");
  DPC_INFO ("Working directory: %s", workdir);

  DPCStatus status = DPC_EFAIL;
  do
    {
      // Set-up DPAL lib.
      if (dpcDpalInit (ini, initialize) < 0)
        {
          break;
        }

      // Set signal handlers.
      if (dpcSetSig () < 0)
        {
          break;
        }

      // Enter main control loop.
      dpcCtrlLoop (workdir, recvid, autostart);

      // Restore original signal handlers.
      dpcUnsetSig ();

      // Clean up DPAL lib.
      dpcDpalDestroy ();

      status = DPC_OK;
    }
  while (FALSE);

  // Remove message queue.
  dpcMsgDestroy (recvid);

  DPC_INFO ("Exiting Daemon Pal control process. Bye!");

  if (status != DPC_OK)
    {
      // Control process could not be set up.
      EXIT_WITH_FAILURE
    }

  // Do not return. Always exit!
  EXIT_WITH_SUCCESS

  return;
}
