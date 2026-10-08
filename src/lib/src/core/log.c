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

#include "core/log.h"
#include "core/def.h"
#include "core/error.h"
#include "plat/alloc.h"
#include "plat/common.h"
#include "plat/filesys.h"
#include "plat/io.h"
#include "plat/logger.h"
#include "plat/str.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define LOG_SUFFIX_MAX 32
#define LOG_EXT ".log"
#define LOG_DIR_MAX 8
#define LOG_MAX ((LOG_DIR_MAX) + (DEF_NAME_MAX) + (LOG_SUFFIX_MAX))
#define BYTE_CHAR_MAX 3
#define ROT_IDX_MAX 255

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct DPLog
{
  DPChar *name;
  DPFd fd;
  DPLogType type;
  DPQword lim;
  DPByte rot;
  DPQword remain;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static const DPChar LOG_DIR[] = "log/";

static const DPChar STDOUT_SUFFIX[] = "-out" LOG_EXT;
static const DPChar STDERR_SUFFIX[] = "-err" LOG_EXT;

COMPILE_ASSERT (SIZEOF (STDOUT_SUFFIX) < (LOG_SUFFIX_MAX) - (BYTE_CHAR_MAX));
COMPILE_ASSERT (SIZEOF (STDERR_SUFFIX) < (LOG_SUFFIX_MAX) - (BYTE_CHAR_MAX));

COMPILE_ASSERT (SIZEOF (LOG_DIR) < LOG_DIR_MAX);

static DPStatus
dpLogRotClean (DPChar *filepath, DPByte from, DPByte to)
{
  DP_DSLOW ("dpLogRotClean: Filepath base: %s", filepath);

  DPSize filepathLen = dpStrnlen (filepath, LOG_MAX);
  dpStrncat (filepath, ".", 1);
  DPChar *end = filepath + filepathLen + 1;

  for (DPInt i = from; i <= to; ++i)
    {
      dpItoa10 (i, end);
      if (dpFileExists (filepath))
        {
          DP_DSLOW ("dpLogRotClean: Unlink: %s", filepath);

          CHECK_REPORT (dpUnlink (filepath) == 0, DP_ELOG);
        }
    }

  filepath[filepathLen] = '\0';

  return DP_OK;
}

static DPChar *
dpStrncatTypeSuffix (DPChar *filepath, DPLogType type)
{
  const DPChar *suffix = type == DP_LOG_STDOUT ? STDOUT_SUFFIX : STDERR_SUFFIX;
  const DPSize suffixLen = type == DP_LOG_STDOUT ? CHARSMAX (STDOUT_SUFFIX)
                                                 : CHARSMAX (STDERR_SUFFIX);

  return dpStrncat (filepath, suffix, suffixLen);
}

static DPStatus
dpLogRot (const DPLog *log)
{
  CHECK_NULL_RETURN (log);
  CHECK_NULL_RETURN (*log);

  CHECK_RETURN (dpIsFdValid ((*log)->fd), DP_ELOG);
  CHECK_RETURN (dpClose ((*log)->fd) == 0, DP_EFD);
  (*log)->fd = DP_FD_INVALID;

  DPChar *filepath = (DPChar *)dpCalloc (LOG_MAX, SIZEOF (DPChar));
  CHECK_ALLOC_RETURN (filepath);

  // Make a log dir path and eventually create the directory.
  dpStrncat (filepath, LOG_DIR, CHARSMAX (LOG_DIR));
  if (!dpDirExists (filepath))
    {
      DP_DSLOW ("dpLogRot: Creating logdir: %s", filepath);

      if (dpMkdir (filepath, DP_S_IRWXU) != 0)
        {
          dpFree (filepath);

          SET_ERROR_RETURN (DP_ELOG,
                            "Condition: (dpMkdir (filepath, S_IRWXU) != 0)");
        }
    }

  // Now, append the log name.
  dpStrncat (filepath, (*log)->name, DEF_NAME_MAX);
  dpStrncatTypeSuffix (filepath, (*log)->type);
  DPSize filepathLen = dpStrnlen (filepath, LOG_MAX);

  // First, check whether we do rotations at all.
  if ((*log)->rot == 0)
    {
      // Remove the current log file.
      if (dpFileExists (filepath))
        {
          DP_DSLOW ("dpLogRot: Unlink: %s", filepath);
          CHECK_REPORT (dpUnlink (filepath) == 0, DP_ELOG);
        }

      // Open the new log file.
      DP_DSLOW ("dpLogRot: Open: %s", filepath);
      (*log)->fd = dpOpen (filepath, DP_O_CREAT | DP_O_APPEND | DP_O_RDWR,
                           DP_S_IRUSR | DP_S_IWUSR);
      dpFree (filepath);
      CHECK_RETURN (dpIsFdValid ((*log)->fd), DP_ELOG);

      (*log)->remain = (*log)->lim;

      return DP_OK;
    }

  // Remove the oldest log rotation.
  DPByte rotIdx = (*log)->rot > 0 ? (*log)->rot - 1 : 0;
  CHECK_STATUS_REPORT (dpLogRotClean (filepath, rotIdx, rotIdx));

  // Shift intermediate rotations.
  for (DPInt i = rotIdx; i > 0; --i)
    {
      DPChar *oldfile = (DPChar *)dpCalloc (LOG_MAX, SIZEOF (DPChar));
      if (oldfile == NULL)
        {
          dpFree (filepath);

          SET_ERROR_RETURN (DP_EALLOC, "Condition: (oldfile == NULL)");
        }

      DPChar *newfile = (DPChar *)dpCalloc (LOG_MAX, SIZEOF (DPChar));
      if (newfile == NULL)
        {
          dpFree (oldfile);
          dpFree (filepath);

          SET_ERROR_RETURN (DP_EALLOC, "Condition: (newfile == NULL)");
        }

      dpStrncat (oldfile, filepath, LOG_MAX);
      dpStrncat (newfile, filepath, LOG_MAX);

      dpStrncat (oldfile, ".", 1);
      dpStrncat (newfile, ".", 1);

      DPChar *oldend = oldfile + filepathLen + 1;
      DPChar *newend = newfile + filepathLen + 1;

      dpItoa10 (i - 1, oldend);
      dpItoa10 (i, newend);

      if (dpFileExists (newfile))
        {
          DP_DSLOW ("dpLogRot: Unlink: %s", newfile);

          CHECK_REPORT (dpUnlink (newfile) == 0, DP_ELOG);
        }

      if (dpFileExists (oldfile))
        {
          DP_DSLOW ("dpLogRot: Rename: %s to: %s", oldfile, newfile);

          CHECK_REPORT (dpRename (oldfile, newfile) == 0, DP_ELOG);
        }

      dpFree (oldfile);
      dpFree (newfile);
    }

  // Create the first rotation.
  DPChar newfile[LOG_MAX] = { 0 };
  dpStrncat (newfile, filepath, LOG_MAX);
  dpStrncat (newfile, ".0", 2);

  DP_DSLOW ("dpLogRot: Rename: %s to: %s", filepath, newfile);
  CHECK_REPORT (dpRename (filepath, newfile) == 0, DP_ELOG);

  // Finally, open the new log file.
  DP_DSLOW ("dpLogRot: Open: %s", filepath);
  (*log)->fd = dpOpen (filepath, DP_O_CREAT | DP_O_APPEND | DP_O_RDWR,
                       DP_S_IRUSR | DP_S_IWUSR);
  dpFree (filepath);
  CHECK_RETURN (dpIsFdValid ((*log)->fd), DP_ELOG);

  (*log)->remain = (*log)->lim;

  return DP_OK;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpLogInit (DPLog *log, const DPChar *name, DPLogType type, DPQword lim,
           DPByte rot)
{
  CHECK_NULL_RETURN (log);
  CHECK_NULL_RETURN (name);

  *log = (DPLog)dpMalloc (SIZEOF (struct DPLog));
  CHECK_ALLOC_RETURN (*log);

  (*log)->name = dpStrndup (name, DEF_NAME_MAX);
  if ((*log)->name == NULL)
    {
      dpFree (*log);

      SET_ERROR_RETURN (DP_EALLOC, "Condition: ((*log)->name == NULL)");
    }
  (*log)->fd = DP_FD_INVALID;
  (*log)->type = type;
  (*log)->lim = lim;
  (*log)->rot = rot;
  (*log)->remain = 0;

  return DP_OK;
}

DPStatus
dpLogDestroy (DPLog *log)
{
  CHECK_NULL_RETURN (log);
  CHECK_NULL_RETURN (*log);

  if ((*log)->name != NULL)
    {
      dpFree ((*log)->name);
    }
  dpFree (*log);

  return DP_OK;
}

DPStatus
dpLogStart (DPLog *log)
{
  CHECK_NULL_RETURN (log);
  CHECK_NULL_RETURN (*log);

  CHECK_RETURN (!dpIsFdValid ((*log)->fd), DP_ELOG);

  DPChar *filepath = (DPChar *)dpCalloc (LOG_MAX, SIZEOF (DPChar));
  CHECK_ALLOC_RETURN (filepath);

  // Make a log dir path and eventually create the directory.
  dpStrncat (filepath, LOG_DIR, CHARSMAX (LOG_DIR));
  if (!dpDirExists (filepath))
    {
      DP_DSLOW ("dpLogStart: Creating logdir: %s", filepath);

      if (dpMkdir (filepath, DP_S_IRWXU) != 0)
        {
          dpFree (filepath);

          SET_ERROR_RETURN (DP_ELOG,
                            "Condition: (dpMkdir (filepath, S_IRWXU) != 0)");
        }
    }

  // Now, append the log name.
  dpStrncat (filepath, (*log)->name, DEF_NAME_MAX);
  dpStrncatTypeSuffix (filepath, (*log)->type);

  (*log)->fd = dpOpen (filepath, DP_O_CREAT | DP_O_APPEND | DP_O_RDWR,
                       DP_S_IRUSR | DP_S_IWUSR);
  if (!dpIsFdValid ((*log)->fd))
    {
      dpFree (filepath);
      SET_ERROR_RETURN (DP_ELOG, "Log condition: (!dpIsFdValid ((*log)->fd))");
    }

  if ((*log)->lim == 0)
    {
      // No log limit implies no log rotation. But we should still clean the
      // old ones.
      CHECK_STATUS_REPORT (dpLogRotClean (filepath, 0, ROT_IDX_MAX));
      dpFree (filepath);

      return DP_OK;
    }

  // Compute remaining capacity and eventually perform log rotation.
  DPQword size = dpFileSize (filepath);
  if (size >= (*log)->lim)
    {
      // No capacity. Perform full log rotation.
      dpFree (filepath);
      CHECK_STATUS_RETURN (dpLogRot (log));

      return DP_OK;
    }

  // Compute remaining size.
  (*log)->remain = (*log)->lim - size;

  // Do clean up.
  CHECK_STATUS_REPORT (dpLogRotClean (filepath, (*log)->rot, ROT_IDX_MAX));
  dpFree (filepath);

  return DP_OK;
}

DPStatus
dpLogStop (DPLog *log)
{
  CHECK_NULL_RETURN (log);
  CHECK_NULL_RETURN (*log);

  CHECK_REPORT (dpIsFdValid ((*log)->fd), DP_ELOG);
  CHECK_REPORT (dpClose ((*log)->fd) == 0, DP_EFD);
  (*log)->fd = DP_FD_INVALID;

  return DP_OK;
}

DPStatus
dpLog (const DPLog *log, const DPChar *data, DPSize size)
{
  CHECK_NULL_RETURN (log);
  CHECK_NULL_RETURN (*log);
  CHECK_NULL_RETURN (data);

  CHECK_RETURN (dpIsFdValid ((*log)->fd), DP_EFD);

  // Check whether the log rotation is disabled.
  if ((*log)->lim == 0)
    {
      CHECK_RETURN (dpWrite ((*log)->fd, data, size) >= 0, DP_ELOG);

      return DP_OK;
    }

  // Check whether we have enough remaining capacity to write. If not,
  // we have to perform log rotation.
  while ((*log)->remain <= size)
    {
      CHECK_RETURN (dpWrite ((*log)->fd, data, (*log)->remain) >= 0, DP_ELOG);
      data += (*log)->remain;
      size -= (*log)->remain;

      CHECK_STATUS_RETURN (dpLogRot (log));
    }

  CHECK_RETURN (dpWrite ((*log)->fd, data, size) >= 0, DP_ELOG);
  (*log)->remain -= size;

  return DP_OK;
}
