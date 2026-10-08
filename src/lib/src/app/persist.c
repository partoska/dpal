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
#include "plat/common.h"
#include "plat/io.h"
#include "plat/logger.h"
#include "plat/str.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define INI_MAX DEF_APP_MAX

#define EKEY_MAX (64)
#define EVAL_MAX ((DEF_ENV_MAX) - (EKEY_MAX) - (1))
#define LINE_MAX (2048)

#define ARG_MAX (32)
#define ENV_MAX (16)
#define ROT_MAX (255)
#define RESTARTSEC_MAX (0xffffffff)
#define ID_MAX (0x7fffffff)

#define SAFE_MARGIN (64)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Asserts
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

COMPILE_ASSERT (DEF_ENV_MAX >= EKEY_MAX + EVAL_MAX + 1);

COMPILE_ASSERT (LINE_MAX >= DEF_NAME_MAX + SAFE_MARGIN);
COMPILE_ASSERT (LINE_MAX >= DEF_APP_MAX + SAFE_MARGIN);
COMPILE_ASSERT (LINE_MAX >= DEF_DIR_MAX + SAFE_MARGIN);
COMPILE_ASSERT (LINE_MAX >= DEF_ARG_MAX + SAFE_MARGIN);
COMPILE_ASSERT (LINE_MAX >= DEF_ENV_MAX + SAFE_MARGIN);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct
{
  DPChar data[LINE_MAX];
  DPSize di;
  DPChar *delim;
} DPIniLine;

typedef struct
{
  DPChar key[EKEY_MAX];
  DPChar value[EVAL_MAX];
} DPEnv;

typedef enum DPEProcProp
{
  ENAME = 0x00000001,
  EID = 0x00000002,
  EAPP = 0x00000004,
  EDIR = 0x00000008,
  EARG = 0x00000010,
  EENV = 0x00000020,
  EMEMLIM = 0x00000040,
  ELOGLIM = 0x00000080,
  ELOGROT = 0x00000100,
  ERESTARTSEC = 0x00000200,
} DPProcProp;

typedef struct
{
  DPChar name[DEF_NAME_MAX];
  DPId id;
  DPChar app[DEF_APP_MAX];
  DPChar dir[DEF_DIR_MAX];
  DPChar args[ARG_MAX][DEF_ARG_MAX];
  DPSize argc;
  DPEnv envs[ENV_MAX];
  DPSize envc;
  DPQword memlim;
  DPQword loglim;
  DPQword logrot;
  DPQword restartsec;
  DPDword flags;
} DPIniProcDesc;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static DPStatus
dpCopyPtrRange (DPChar *target, DPSize n, const DPChar *start,
                const DPChar *end)
{
  CHECK_NULL_RETURN (target);
  CHECK_NULL_RETURN (start);
  CHECK_NULL_RETURN (end);
  CHECK_RETURN (start <= end, DP_ECONF);

  DPSize len = end - start;

  // Always ensure trailing zero.
  CHECK_RETURN (len < n, DP_ECONF);
  dpMemcpy (target, start, len);
  target[len] = '\0';

  return DP_OK;
}

// ===============================================================
// Line manipulation.
// ===============================================================

static DPStatus
dpLineInit (DPIniLine *line)
{
  CHECK_NULL_RETURN (line);

  line->data[0] = '\0';
  line->di = 0;
  line->delim = NULL;

  return DP_OK;
}

static DPStatus
dpLinePutc (DPIniLine *line, DPChar c)
{
  CHECK_NULL_RETURN (line);
  CHECK_RETURN (line->di < CHARSMAX (line->data), DP_ECONF);

  // Put character into buffer.
  line->data[line->di] = c;

  // If this is first '=' character mark it as delimiter.
  if (line->delim == NULL && c == '=')
    {
      line->delim = &line->data[line->di];
    }

  // Move forward inside the buffer.
  ++line->di;

  return DP_OK;
}

static DPStatus
dpLineTerm (DPIniLine *line)
{
  CHECK_NULL_RETURN (line);

  line->data[line->di] = '\0';

  return DP_OK;
}

static inline const DPChar *
dpLineFirst (const DPIniLine *line)
{
  return &line->data[0];
}

static inline const DPChar *
dpLineLast (const DPIniLine *line)
{
  return &line->data[line->di];
}

static inline DPChar *
dpIniLineDelim (const DPIniLine *line)
{
  return line->delim;
}

static inline DPSize
dpLineLen (const DPIniLine *line)
{
  return line->di;
}

static inline const DPChar *
dpLineData (const DPIniLine *line)
{
  return line->data;
}

// ===============================================================
// Process description manipulation.
// ===============================================================

static DPStatus
dpDescInit (DPIniProcDesc *desc)
{
  CHECK_NULL_RETURN (desc);

  desc->name[0] = '\0';
  desc->id = 0;
  desc->app[0] = '\0';
  desc->dir[0] = '\0';
  for (DPSize i = 0; i < ARG_MAX; ++i)
    {
      desc->args[i][0] = '\0';
    }
  desc->argc = 0;
  for (DPSize i = 0; i < ENV_MAX; ++i)
    {
      desc->envs[i].key[0] = '\0';
      desc->envs[i].value[0] = '\0';
    }
  desc->envc = 0;
  desc->memlim = 0;
  desc->loglim = 0;
  desc->logrot = 1;
  desc->restartsec = 0;
  desc->flags = 0;

  return DP_OK;
}

static inline void
dpDescSetFlag (DPIniProcDesc *desc, DPProcProp flag)
{
  desc->flags |= flag;
}

static inline DPBool
dpDescHasFlag (const DPIniProcDesc *desc, DPProcProp flag)
{
  return desc->flags & flag;
}

// ===============================================================
// Process registration.
// ===============================================================

static DPStatus
dpDescReg (DPIniProcDesc *desc)
{
  CHECK_NULL_RETURN (desc);

  if (!dpDescHasFlag (desc, ENAME))
    {
      // Name not set. Nothing to register.
      return DP_OK;
    }

  DP_DEBUG ("Registering process from INI: %s", desc->name);

  // Name is set. Check for other mandatory properties.
  // NOTE: Id is zero by default (which is ok).
  CHECK_RETURN (dpDescHasFlag (desc, EAPP), DP_ECONF);
  CHECK_RETURN (dpDescHasFlag (desc, EDIR), DP_ECONF);
  // At least argv[0] must be always set.
  CHECK_RETURN (dpDescHasFlag (desc, EARG), DP_ECONF);

  // Create processs definition and register it.
  DPProcDef def;
  CHECK_STATUS_RETURN (
      dpProcDefInitEx (&def, desc->name, desc->app, desc->dir, NULL, NULL));
  for (DPSize i = 0; i < desc->argc; ++i)
    {
      DPStatus status = dpProcDefAddArg (&def, desc->args[i]);
      if (status != DP_OK)
        {
          CHECK_STATUS_REPORT (dpProcDefDestroy (&def));

          return status;
        }
    }
  for (DPSize i = 0; i < desc->envc; ++i)
    {
      DPStatus status
          = dpProcDefAddEnv (&def, desc->envs[i].key, desc->envs[i].value);
      if (status != DP_OK)
        {
          CHECK_STATUS_REPORT (dpProcDefDestroy (&def));

          return status;
        }
    }
  DPId id = desc->id;

  DPProcAttr attr;
  DPStatus status = dpProcAttrInit (&attr);
  if (status != DP_OK)
    {
      CHECK_STATUS_REPORT (dpProcDefDestroy (&def));

      return status;
    }

  status = dpProcAttrSetMaxMem (&attr, desc->memlim);
  if (status != DP_OK)
    {
      CHECK_STATUS_REPORT (dpProcAttrDestroy (&attr));
      CHECK_STATUS_REPORT (dpProcDefDestroy (&def));

      return status;
    }

  status = dpProcAttrSetLogMaxSize (&attr, desc->loglim);
  if (status != DP_OK)
    {
      CHECK_STATUS_REPORT (dpProcAttrDestroy (&attr));
      CHECK_STATUS_REPORT (dpProcDefDestroy (&def));

      return status;
    }

  status = dpProcAttrSetLogRot (&attr, desc->logrot);
  if (status != DP_OK)
    {
      CHECK_STATUS_REPORT (dpProcAttrDestroy (&attr));
      CHECK_STATUS_REPORT (dpProcDefDestroy (&def));

      return status;
    }

  status = dpProcAttrSetRestartSec (&attr, (DPDword)desc->restartsec);
  if (status != DP_OK)
    {
      CHECK_STATUS_REPORT (dpProcAttrDestroy (&attr));
      CHECK_STATUS_REPORT (dpProcDefDestroy (&def));

      return status;
    }

  status = dpProcReg (&id, &def, &attr);
  if (status != DP_OK)
    {
      CHECK_STATUS_REPORT (dpProcAttrDestroy (&attr));
      CHECK_STATUS_REPORT (dpProcDefDestroy (&def));

      return status;
    }

  CHECK_STATUS_REPORT (dpProcAttrDestroy (&attr));
  CHECK_STATUS_REPORT (dpProcDefDestroy (&def));

  return DP_OK;
}

// ===============================================================
// INI file parsing.
// ===============================================================

static DPStatus
dpParseEnv (DPIniProcDesc *desc, const DPChar *senv, const DPChar *eenv)
{
  CHECK_NULL_RETURN (desc);
  CHECK_NULL_RETURN (senv);
  CHECK_NULL_RETURN (eenv);
  CHECK_RETURN (senv < eenv, DP_ECONF);
  CHECK_RETURN (*senv == '(' && *(eenv - 1) == ')', DP_ECONF);

  // Skip starting and trailing parens.
  ++senv;
  --eenv;

  // Match env name.
  const DPChar *match = "Name=";
  const DPChar *ptr = senv;
  DPSize i = 0;
  while (match[i] != '\0')
    {
      CHECK_RETURN (*ptr == match[i], DP_ECONF);

      ++ptr;
      ++i;
    }

  // Read env name.
  DPSize j = 0;
  while (*ptr != ',')
    {
      CHECK_RETURN (j < EKEY_MAX - 1, DP_ECONF);

      desc->envs[desc->envc].key[j] = *ptr;

      ++ptr;
      ++j;
    }
  desc->envc[desc->envs].key[j] = '\0';

  // Skip delimiter ','.
  ++ptr;

  // Match env value.
  match = "Value=";
  i = 0;
  while (match[i] != '\0')
    {
      CHECK_RETURN (*ptr == match[i], DP_ECONF);

      ++ptr;
      ++i;
    }

  // Read env value.
  j = 0;
  while (ptr != eenv)
    {
      CHECK_RETURN (j < EVAL_MAX - 1, DP_ECONF);

      desc->envc[desc->envs].value[j] = *ptr;

      ++ptr;
      ++j;
    }
  desc->envs[desc->envc].value[j] = '\0';

  return DP_OK;
}

static DPStatus
dpParseKeyVal (DPIniProcDesc *desc, const DPIniLine *line)
{
  CHECK_NULL_RETURN (desc);
  CHECK_NULL_RETURN (line);

  // The delimiter '=' has to be a part of the line.
  CHECK_RETURN (dpIniLineDelim (line) != NULL, DP_ECONF);

  const DPChar *sk = dpLineFirst (line);
  const DPChar *ek = dpIniLineDelim (line);
  DPSize len = ek - sk;
  CHECK_RETURN (sk < ek, DP_ECONF);

  const DPChar *sv = dpIniLineDelim (line) + 1;
  const DPChar *ev = dpLineLast (line);
  CHECK_RETURN (sv < ev, DP_ECONF);

  if (dpStrncmp (sk, "App", len) == 0)
    {
      if (dpDescHasFlag (desc, EAPP))
        {
          DP_WARN ("Duplicate application entry in INI file for process %s",
                   desc->name);
        }

      CHECK_STATUS_RETURN (
          dpCopyPtrRange (desc->app, SIZEOF (desc->app), sv, ev));

      dpDescSetFlag (desc, EAPP);

      DP_DSLOW ("dpParseKeyVal: App=%s", desc->app);

      return DP_OK;
    }

  if (dpStrncmp (sk, "Dir", len) == 0)
    {
      if (dpDescHasFlag (desc, EDIR))
        {
          DP_WARN ("Duplicate directory entry in INI file for process %s",
                   desc->name);
        }

      CHECK_STATUS_RETURN (
          dpCopyPtrRange (desc->dir, SIZEOF (desc->dir), sv, ev));

      dpDescSetFlag (desc, EDIR);

      DP_DSLOW ("dpParseKeyVal: Dir=%s", desc->dir);

      return DP_OK;
    }

  if (dpStrncmp (sk, "Id", len) == 0)
    {
      if (dpDescHasFlag (desc, EID))
        {
          DP_WARN ("Duplicate id entry in INI file for process %s",
                   desc->name);
        }

      DPChar buff[32] = { 0 };
      CHECK_STATUS_RETURN (dpCopyPtrRange (buff, SIZEOF (buff), sv, ev));
      desc->id = dpStrtoul (buff, NULL, 10);
      CHECK_RETURN (desc->id >= 0, DP_ECONF);
      CHECK_RETURN (desc->id <= ID_MAX, DP_ECONF);

      dpDescSetFlag (desc, EID);

      DP_DSLOW ("dpParseKeyVal: Id=%d", desc->id);

      return DP_OK;
    }

  if (dpStrncmp (sk, "Arg", len) == 0)
    {
      CHECK_RETURN (desc->argc < ARG_MAX, DP_ECONF);
      CHECK_STATUS_RETURN (dpCopyPtrRange (desc->args[desc->argc],
                                           SIZEOF (desc->args[0]), sv, ev));

      dpDescSetFlag (desc, EARG);

      DP_DSLOW ("dpParseKeyVal: Arg=%s", desc->args[desc->argc]);

      ++desc->argc;

      return DP_OK;
    }

  if (dpStrncmp (sk, "Env", len) == 0)
    {
      CHECK_RETURN (desc->envc < ENV_MAX, DP_ECONF);
      CHECK_STATUS_RETURN (dpParseEnv (desc, sv, ev));

      dpDescSetFlag (desc, EENV);

      DP_DSLOW ("dpParseKeyVal: Env=(Name=%s,Value=%s)",
                desc->envs[desc->envc].key, desc->envs[desc->envc].value);

      ++desc->envc;

      return DP_OK;
    }

  if (dpStrncmp (sk, "MaxMemory", len) == 0)
    {
      if (dpDescHasFlag (desc, EMEMLIM))
        {
          DP_WARN ("Duplicate max memory size entry in INI file for "
                   "process %s",
                   desc->name);
        }

      DPChar buff[32] = { 0 };
      CHECK_STATUS_RETURN (dpCopyPtrRange (buff, SIZEOF (buff), sv, ev));
      desc->memlim = dpStrtoul (buff, NULL, 10);

      dpDescSetFlag (desc, EMEMLIM);

      DP_DSLOW ("dpParseKeyVal: MaxMemory=%lu", desc->memlim);

      return DP_OK;
    }

  if (dpStrncmp (sk, "LogMaxSize", len) == 0)
    {
      if (dpDescHasFlag (desc, ELOGLIM))
        {
          DP_WARN ("Duplicate log maximum size entry in INI file for "
                   "process %s",
                   desc->name);
        }

      DPChar buff[32] = { 0 };
      CHECK_STATUS_RETURN (dpCopyPtrRange (buff, SIZEOF (buff), sv, ev));
      desc->loglim = dpStrtoul (buff, NULL, 10);

      dpDescSetFlag (desc, ELOGLIM);

      DP_DSLOW ("dpParseKeyVal: LogMaxSize=%lu", desc->loglim);

      return DP_OK;
    }

  if (dpStrncmp (sk, "LogRotation", len) == 0)
    {
      if (dpDescHasFlag (desc, ELOGROT))
        {
          DP_WARN ("Duplicate log rotations entry in INI file for process "
                   "%s",
                   desc->name);
        }

      DPChar buff[32] = { 0 };
      CHECK_STATUS_RETURN (dpCopyPtrRange (buff, SIZEOF (buff), sv, ev));
      desc->logrot = dpStrtoul (buff, NULL, 10);
      CHECK_RETURN (desc->logrot <= ROT_MAX, DP_ECONF);

      dpDescSetFlag (desc, ELOGROT);

      DP_DSLOW ("dpParseKeyVal: LogRotation=%u", desc->logrot);

      return DP_OK;
    }

  if (dpStrncmp (sk, "RestartSec", len) == 0)
    {
      if (dpDescHasFlag (desc, ERESTARTSEC))
        {
          DP_WARN ("Duplicate restart seconds entry in INI file for "
                   "process %s",
                   desc->name);
        }

      DPChar buff[32] = { 0 };
      CHECK_STATUS_RETURN (dpCopyPtrRange (buff, SIZEOF (buff), sv, ev));
      desc->restartsec = dpStrtoul (buff, NULL, 10);
      CHECK_RETURN (desc->restartsec <= RESTARTSEC_MAX, DP_ECONF);

      dpDescSetFlag (desc, ERESTARTSEC);

      DP_DSLOW ("dpParseKeyVal: RestartSec=%lu", desc->restartsec);

      return DP_OK;
    }

  DP_WARN ("Unexpected entry in INI file for process "
           "%s: %s",
           desc->name, dpLineData (line));

  return DP_OK;
}

static DPStatus
dpParseLine (DPIniProcDesc *desc, const DPIniLine *line)
{
  CHECK_NULL_RETURN (desc);
  CHECK_NULL_RETURN (line);

  // Skip empty lines and comments.
  if (dpLineLen (line) == 0 || *dpLineFirst (line) == '\0'
      || *dpLineFirst (line) == '#' || *dpLineFirst (line) == ';')
    {
      return DP_OK;
    }

  // Check for proc section header.
  if (*dpLineFirst (line) == '[')
    {
      // Firstly, try to register process defined in the previous section.
      CHECK_STATUS_RETURN (dpDescReg (desc));

      // Clear any previous properties obtained from the INI file.
      CHECK_STATUS_RETURN (dpDescInit (desc));

      const DPChar *sname = dpLineFirst (line);
      const DPChar *ename = dpLineLast (line);
      CHECK_RETURN (*(ename - 1) == ']', DP_ECONF);

      // Skip starting and trailing parens.
      ++sname;
      --ename;

      // Read process name.
      const DPChar *ptr = sname;
      DPSize i = 0;
      while (ptr != ename)
        {
          CHECK_RETURN (i < DEF_NAME_MAX - 1, DP_ECONF);

          desc->name[i] = *ptr;

          ++ptr;
          ++i;
        }
      desc->name[i] = '\0';

      dpDescSetFlag (desc, ENAME);

      DP_DSLOW ("dpParseLine: [%s]", desc->name);

      return DP_OK;
    }

  // This should be a key-value pair for the current section.
  CHECK_STATUS_RETURN (dpParseKeyVal (desc, line));

  return DP_OK;
}

static DPStatus
dpParseChar (DPIniProcDesc *desc, DPIniLine *line, DPChar c)
{
  CHECK_NULL_RETURN (line);

  if (c != '\n')
    {
      CHECK_STATUS_RETURN (dpLinePutc (line, c));

      return DP_OK;
    }

  CHECK_STATUS_RETURN (dpLineTerm (line));
  CHECK_STATUS_RETURN (dpParseLine (desc, line));
  CHECK_STATUS_RETURN (dpLineInit (line));

  return DP_OK;
}

static DPStatus
dpParseFd (DPFd fd)
{
  static DPChar buff[256] = { 0 };
  static DPIniLine line;

  static DPIniProcDesc desc;
  CHECK_STATUS_REPORT (dpDescInit (&desc));

  DPInt bytes;
  do
    {
      bytes = dpRead (fd, buff, SIZEOF (buff));
      CHECK_RETURN (bytes >= 0, DP_EREAD);
      for (DPInt i = 0; i < bytes; ++i)
        {
          CHECK_STATUS_RETURN (dpParseChar (&desc, &line, buff[i]));
        }
    }
  while (bytes > 0);

  // Finally, try to register process defined the very last section.
  CHECK_STATUS_RETURN (dpDescReg (&desc));

  return DP_OK;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpProcLoad (const DPChar *ini)
{
  CHECK_INIT_RETURN ();
  CHECK_NULL_RETURN (ini);

  DP_INFO ("Loading process definitions from: %s", ini);

  DPSize len = dpStrnlen (ini, INI_MAX);
  CHECK_RETURN (len < INI_MAX, DP_ETOOLONG);
  CHECK_RETURN (len > 0, DP_EINVALID);

  DPFd fd = dpOpen (ini, DP_O_RDONLY, 0);
  CHECK_RETURN (dpIsFdValid (fd), DP_ENOT_FOUND);
  DPStatus status = dpParseFd (fd);
  if (status != DP_OK)
    {
      CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);

      return status;
    }

  CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);

  return DP_OK;
}

DPStatus
dpProcSave (const DPChar *ini)
{
  CHECK_INIT_RETURN ();
  CHECK_NULL_RETURN (ini);

  DP_INFO ("Saving process definitions to: %s", ini);

  DPSize len = dpStrnlen (ini, INI_MAX);
  CHECK_RETURN (len < INI_MAX, DP_ETOOLONG);
  CHECK_RETURN (len > 0, DP_EINVALID);

  DPProcIter iter;
  CHECK_STATUS_RETURN (dpProcIter (&iter));

  DPFd fd = dpOpen (ini, DP_O_CREAT | DP_O_TRUNC | DP_O_WRONLY,
                    DP_S_IRUSR | DP_S_IWUSR);
  CHECK_RETURN (dpIsFdValid (fd), DP_ENOT_FOUND);

  dpPrint (fd, "; This file was generated by Daemon Pal v" DP_VERSION ".\n");
  dpPrint (fd, "; More info at <https://lab.partoska.com/dpal>\n");
  dpPrint (fd, "\n");

  DPId next;
  while (dpProcIterNext (&iter, &next) != DP_BREAK)
    {
      static DPChar buff[LINE_MAX] = { 0 };
      DPStatus status;

      // =====================================================
      // Definition.
      // =====================================================

      DPProcDef def;
      if ((status = dpProcGetDef (&def, next)) != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }

      if ((status = dpProcDefGetName (&def, buff, CHARSMAX (buff), NULL))
          != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }
      dpPrint (fd, "[%s]\n", buff);

      dpPrint (fd, "Id=%d\n", next);

      if ((status = dpProcDefGetApp (&def, buff, CHARSMAX (buff), NULL))
          != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }
      dpPrint (fd, "App=%s\n", buff);

      if ((status = dpProcDefGetDir (&def, buff, CHARSMAX (buff), NULL))
          != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }
      dpPrint (fd, "Dir=%s\n", buff);

      DPSize cnt;
      if ((status = dpProcDefGetArgc (&def, &cnt)) != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }
      for (DPSize i = 0; i < cnt; ++i)
        {
          if ((status = dpProcDefGetArg (&def, buff, i, CHARSMAX (buff), NULL))
              != DP_OK)
            {
              CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
              return status;
            }
          dpPrint (fd, "Arg=%s\n", buff);
        }

      if ((status = dpProcDefGetEnvc (&def, &cnt)) != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }
      for (DPSize i = 0; i < cnt; ++i)
        {
          if ((status
               = dpProcDefGetEnvName (&def, buff, i, CHARSMAX (buff), NULL))
              != DP_OK)
            {
              CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
              return status;
            }
          dpPrint (fd, "Env=(Name=%s,", buff);

          if ((status
               = dpProcDefGetEnvVal (&def, buff, i, CHARSMAX (buff), NULL))
              != DP_OK)
            {
              CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
              return status;
            }
          dpPrint (fd, "Value=%s)\n", buff);
        }

      CHECK_STATUS_REPORT (dpProcDefDestroy (&def));

      // =====================================================
      // Attributes.
      // =====================================================

      DPProcAttr attr;
      if ((status = dpProcGetAttr (&attr, next)) != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }

      DPQword mem;
      if ((status = dpProcAttrGetMaxMem (&attr, &mem)) != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }
      if (mem != 0)
        {
          // Do not print defaults.
          dpPrint (fd, "MaxMemory=%lu\n", mem);
        }

      DPSize lim;
      if ((status = dpProcAttrGetLogMaxSize (&attr, &lim)) != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }
      if (lim != 0)
        {
          // Do not print defaults.
          dpPrint (fd, "LogMaxSize=%lu\n", lim);
        }

      DPByte rot;
      if ((status = dpProcAttrGetLogRot (&attr, &rot)) != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }
      if (rot != 1)
        {
          // Do not print defaults.
          dpPrint (fd, "LogRotation=%u\n", rot);
        }

      DPDword sec;
      if ((status = dpProcAttrGetRestartSec (&attr, &sec)) != DP_OK)
        {
          CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);
          return status;
        }
      if (sec != 0)
        {
          // Do not print defaults.
          dpPrint (fd, "RestartSec=%u\n", sec);
        }

      CHECK_STATUS_REPORT (dpProcAttrDestroy (&attr));

      dpPrint (fd, "\n");
    }

  CHECK_REPORT (dpClose (fd) == 0, DP_ECLOSE);

  return DP_OK;
}
