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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>

#include "plat.h"

#include "logger.h"
#include "plat/env.h"
#include "plat/filesys.h"
#include "plat/fork.h"
#include "plat/io.h"
#include "plat/str.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// ===============================================================
// Alloc
// ===============================================================

void
dpcFree (void *ptr)
{
  free (ptr);
}

// ===============================================================
// Strings
// ===============================================================

void *
dpcMemcpy (void *restrict dst, const void *restrict src, DPSize n)
{
  return dpMemcpy (dst, src, n);
}

DPSize
dpcStrlen (const DPChar *src)
{
  return dpStrlen (src);
}

DPSize
dpcStrnlen (const DPChar *src, DPSize max)
{
  return dpStrnlen (src, max);
}

DPChar *
dpcStrncpy (DPChar *dst, const DPChar *src, DPSize n)
{
  return dpStrncpy (dst, src, n);
}

DPChar *
dpcStrndup (const DPChar *src, DPSize n)
{
  return dpStrndup (src, n);
}

DPChar *
dpcStrncat (DPChar *dst, const DPChar *src, DPSize n)
{
  return dpStrncat (dst, src, n);
}

DPInt
dpcStrcmp (const DPChar *src1, const DPChar *src2)
{
  return dpStrcmp (src1, src2);
}

DPInt
dpcAtoi (const DPChar *num)
{
  return atoi (num);
}

DPChar *
dpcBasename (DPChar *path)
{
  return dpBasename (path);
}

DPChar *
dpcItoa10 (DPInt num, DPChar *str)
{
  return dpItoa10 (num, str);
}

// ===============================================================
// Program flow
// ===============================================================

void
dpcExit (DPInt status)
{
  dpExit (status);
}

DPDword
dpcSleep (DPDword sec)
{
  return sleep (sec);
}

DPInt
dpcSigIntHandler (void (*handler) (DPInt))
{
  struct sigaction act;
  dpMemset (&act, 0x0, SIZEOF (act));
  act.sa_handler = handler != NULL ? handler : SIG_DFL;

  return sigaction (SIGINT, &act, NULL);
}

DPInt
dpcSigTermHandler (void (*handler) (DPInt))
{
  struct sigaction act;
  dpMemset (&act, 0x0, SIZEOF (act));
  act.sa_handler = handler != NULL ? handler : SIG_DFL;

  return sigaction (SIGTERM, &act, NULL);
}

// ===============================================================
// File system
// ===============================================================

DPChar *
dpcGetcwd (DPChar *dir, DPSize n)
{
  return getcwd (dir, n);
}

DPInt
dpcChdir (const DPChar *path)
{
  return dpChdir (path);
}

DPBool
dpcDirExists (const DPChar *dir)
{
  return dpDirExists (dir);
}

DPBool
dpcFileExists (const DPChar *file)
{
  return dpFileExists (file);
}

DPInt
dpcMkdir (const DPChar *path)
{
  return dpMkdir (path, S_IRWXU);
}

DPInt
dpcUnlink (const DPChar *path)
{
  return dpUnlink (path);
}

DPChar *
dpcRealpath (const DPChar *restrict path, DPChar *restrict resolved)
{
  return realpath (path, resolved);
}

DPChar *
dpcRealpathne (const DPChar *path)
{
  // Create a copy of the path since we will need to modify it.
  DPChar *cdir = strndup (path, PATH_MAX);
  if (!cdir)
    {
      return NULL;
    }

  // Get the directory part of the path.
  DPChar *dname = dirname (cdir);

  // Resolve the directory realpath.
  DPChar *dir = realpath (dname, NULL);

  // Copy is no longer needed.
  free (cdir);

  // If directory does not exist, handle error.
  if (dir == NULL)
    {
      return NULL;
    }

  // Make another copy for basename.
  char *cbase = strndup (path, PATH_MAX);
  if (!cbase)
    {
      free (dir);

      return NULL;
    }

  // Get the filename part.
  char *fname = basename (cbase);
  if (strcmp (".", fname) == 0 || strcmp ("..", fname) == 0
      || strcmp ("/", fname) == 0)
    {
      free (dir);
      free (cbase);

      return NULL;
    }

  size_t dir_len = strnlen (dir, PATH_MAX);
  size_t fname_len = strnlen (fname, PATH_MAX);
  if (dir_len + fname_len + 1 >= PATH_MAX)
    {
      free (dir);
      free (cbase);

      return NULL;
    }

  // Combine resolved directory with filename.
  DPChar *result = (DPChar *)malloc (PATH_MAX);
  if (result == NULL)
    {
      free (dir);
      free (cbase);

      return NULL;
    }
  memset (result, 0x0, PATH_MAX);

  // One can safely use strcat instead of strncpy because we have already
  // checked the total lenght.
  strcat (result, dir);
  if (dir_len > 0 && dir[dir_len - 1] != '/')
    {
      strcat (result, "/");
      strcat (result, fname);
    }
  else
    {
      strcat (result, fname);
    }

  free (dir);
  free (cbase);

  return result;
}

// ===============================================================
// Environment
// ===============================================================

DPChar *
dpcGetenv (const DPChar *name)
{
  return dpGetenv (name);
}

DPInt
dpcPutenv (DPChar *env)
{
  return dpPutenv (env);
}

// ===============================================================
// Command line options
// ===============================================================

DPInt
dpcGetoptLong (DPInt argc, DPChar *argv[], const DPChar *optstring,
               const struct DPCOption *longopts, DPInt *longindex)
{
  opterr = 0;

  return getopt_long (argc, argv, optstring, (const struct option *)longopts,
                      longindex);
}

DPChar *
dpcOptArg (void)
{
  return optarg;
}

DPInt
dpcOptInd (void)
{
  return optind;
}

// ===============================================================
// Messages
// ===============================================================

DPInt
dpcFtok (const DPChar *pathname, DPInt projId)
{
  return ftok (pathname, projId);
}

DPInt
dpcMsgGet (DPInt key, DPBool creat, DPBool excl)
{
  DPInt flags = 0600;
  if (creat)
    {
      flags |= IPC_CREAT;
    }

  if (excl)
    {
      flags |= IPC_EXCL;
    }

  return msgget (key, flags);
}

DPInt
dpcMsgSnd (DPInt msgid, const void *msgp, DPSize msgsz)
{
  return msgsnd (msgid, msgp, msgsz, 0);
}

DPSSize
dpcMsgRcv (DPInt msgid, void *msgp, DPSize msgsz, DPLong msgtyp)
{
  errno = 0;
  DPSSize result = msgrcv (msgid, msgp, msgsz, msgtyp, IPC_NOWAIT);
  if (result >= 0)
    {
      return result;
    }

  if (errno == ENOMSG)
    {
      return 0;
    }

  return result;
}

DPInt
dpcMsgRm (DPInt msgid)
{
  return msgctl (msgid, IPC_RMID, NULL);
}
