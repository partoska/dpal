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

#include "core/def.h"
#include "core/error.h"
#include "plat/alloc.h"
#include "plat/common.h"
#include "plat/str.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct DPSProcDef
{
  DPChar *name;
  DPChar *app;
  DPChar *dir;
  DPSize argc;
  DPChar **argv;
  DPSize envc;
  DPChar **envv;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static DPStatus
dpCheckName (const DPChar *name)
{
  CHECK_NULL_RETURN (name);

  CHECK_RETURN ((name[0] >= 'A' && name[0] <= 'Z')
                    || (name[0] >= 'a' && name[0] <= 'z'),
                DP_EINVALID);

  DPSize i = 0;
  while (name[i] != '\0')
    {
      do
        {
          if (name[i] >= 'A' && name[i] <= 'Z')
            {
              break;
            }

          if (name[i] >= 'a' && name[i] <= 'z')
            {
              break;
            }

          if (name[i] >= '0' && name[i] <= '9')
            {
              break;
            }

          if (name[i] == '_' || name[i] == '-')
            {
              break;
            }

          SET_ERROR_RETURN (DP_EINVALID, "Name contains invalid chars");
        }
      while (FALSE);

      ++i;
    }

  return DP_OK;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpProcDefInit (DPProcDef *def, const DPChar *name, const DPChar *app,
               const DPChar *dir, const DPChar *const args[],
               const DPChar *const envs[])
{
  CHECK_NULL_RETURN (def);
  CHECK_NULL_RETURN (name);
  CHECK_NULL_RETURN (app);
  CHECK_NULL_RETURN (dir);
  CHECK_NULL_RETURN (args);
  CHECK_NULL_RETURN (envs);

  CHECK_STATUS_RETURN (dpProcDefInitEx (def, name, app, dir, args, envs));

  return DP_OK;
}

DPStatus
dpProcDefDestroy (DPProcDef *def)
{
  CHECK_DEF_RETURN (def);

  for (DPSize i = 0; i < (*def)->envc; ++i)
    {
      if ((*def)->envv[i] != NULL)
        {
          dpFree ((*def)->envv[i]);
        }
    }

  if ((*def)->envv != NULL)
    {
      dpFree ((*def)->envv);
    }

  for (DPSize i = 0; i < (*def)->argc; ++i)
    {
      if ((*def)->argv[i] != NULL)
        {
          dpFree ((*def)->argv[i]);
        }
    }

  if ((*def)->argv != NULL)
    {
      dpFree ((*def)->argv);
    }

  if ((*def)->dir != NULL)
    {
      dpFree ((*def)->dir);
    }

  if ((*def)->app != NULL)
    {
      dpFree ((*def)->app);
    }

  if ((*def)->name != NULL)
    {
      dpFree ((*def)->name);
    }

  dpFree (*def);

  *def = NULL;

  return DP_OK;
}

DPStatus
dpProcDefGetName (const DPProcDef *def, DPChar *name, DPSize sz, DPSize *used)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (name);

  dpStrncpy (name, (*def)->name, sz);
  if (used != NULL)
    {
      *used = MIN (dpStrlen ((*def)->name) + 1, sz);
    }

  return DP_OK;
}

DPStatus
dpProcDefGetApp (const DPProcDef *def, DPChar *app, DPSize sz, DPSize *used)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (app);

  dpStrncpy (app, (*def)->app, sz);
  if (used != NULL)
    {
      *used = MIN (dpStrlen ((*def)->app) + 1, sz);
    }

  return DP_OK;
}

DPStatus
dpProcDefGetDir (const DPProcDef *def, DPChar *dir, DPSize sz, DPSize *used)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (dir);

  dpStrncpy (dir, (*def)->dir, sz);
  if (used != NULL)
    {
      *used = MIN (dpStrlen ((*def)->dir) + 1, sz);
    }

  return DP_OK;
}

DPStatus
dpProcDefGetArgc (const DPProcDef *def, DPSize *argc)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (argc);

  *argc = (*def)->argc;

  return DP_OK;
}

DPStatus
dpProcDefGetArg (const DPProcDef *def, DPChar *arg, DPByte idx, DPSize sz,
                 DPSize *used)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (arg);

  CHECK_RETURN (idx < (*def)->argc, DP_EBOUND);

  dpStrncpy (arg, (*def)->argv[idx], sz);
  if (used != NULL)
    {
      *used = MIN (dpStrlen ((*def)->argv[idx]) + 1, sz);
    }

  return DP_OK;
}

DPStatus
dpProcDefGetEnvc (const DPProcDef *def, DPSize *envc)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (envc);

  *envc = (*def)->envc;

  return DP_OK;
}

DPStatus
dpProcDefGetEnv (const DPProcDef *def, DPChar *env, DPByte idx, DPSize sz,
                 DPSize *used)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (env);

  CHECK_RETURN (idx < (*def)->envc, DP_EBOUND);

  dpStrncpy (env, (*def)->envv[idx], sz);
  if (used != NULL)
    {
      *used = MIN (dpStrlen ((*def)->envv[idx]) + 1, sz);
    }

  return DP_OK;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpProcDefInitEx (DPProcDef *def, const DPChar *name, const DPChar *app,
                 const DPChar *dir, const DPChar *const args[],
                 const DPChar *const envs[])
{
  CHECK_NULL_RETURN (def);
  CHECK_NULL_RETURN (name);
  CHECK_NULL_RETURN (app);
  CHECK_NULL_RETURN (dir);

  DPSize nameLen = dpStrnlen (name, DEF_NAME_MAX);
  CHECK_RETURN (nameLen < DEF_NAME_MAX, DP_ETOOLONG);
  CHECK_RETURN (nameLen > 0, DP_EINVALID);
  CHECK_STATUS_RETURN (dpCheckName (name));

  DPSize appLen = dpStrnlen (app, DEF_APP_MAX);
  CHECK_RETURN (appLen < DEF_APP_MAX, DP_ETOOLONG);
  CHECK_RETURN (appLen > 0, DP_EINVALID);

  DPChar *appBasename = dpStrndup (app, DEF_APP_MAX);
  const DPChar *basename = dpBasename (appBasename);
  if (dpStrcmp (basename, "/") == 0 || dpStrcmp (basename, ".") == 0
      || dpStrcmp (basename, "..") == 0)
    {
      dpFree (appBasename);

      SET_ERROR_RETURN (DP_EINVALID, "App basename refers to a directory");
    }
  dpFree (appBasename);

  DPSize dirLen = dpStrnlen (dir, DEF_DIR_MAX);
  CHECK_RETURN (dirLen < DEF_DIR_MAX, DP_ETOOLONG);
  CHECK_RETURN (dirLen > 0, DP_EINVALID);

  DPSize i = 0;
  if (args != NULL)
    {
      while (args[i] != NULL)
        {
          DPSize argLen = dpStrnlen (args[i], DEF_ARG_MAX);
          CHECK_RETURN (argLen < DEF_ARG_MAX, DP_ETOOLONG);
          CHECK_RETURN (argLen > 0, DP_EINVALID);

          ++i;
        }
    }
  DPSize argc = i;

  DPSize k = 0;
  if (envs != NULL)
    {
      while (envs[k] != NULL)
        {
          DPSize envLen = dpStrnlen (envs[k], DEF_ENV_MAX);
          CHECK_RETURN (envLen < DEF_ENV_MAX, DP_ETOOLONG);
          CHECK_RETURN (envLen > 0, DP_EINVALID);

          ++k;
        }
    }
  DPSize envc = k;

  *def = (DPProcDef)dpMalloc (SIZEOF (struct DPSProcDef));
  CHECK_ALLOC_RETURN (*def);

  DPChar *nameCopy = dpStrndup (name, DEF_NAME_MAX);
  if (nameCopy == NULL)
    {
      dpFree (*def);

      SET_ERROR_RETURN (DP_EALLOC, "Condition: (nameCopy == NULL)");
    }

  DPChar *appCopy = dpStrndup (app, DEF_APP_MAX);
  if (appCopy == NULL)
    {
      dpFree (nameCopy);
      dpFree (*def);

      SET_ERROR_RETURN (DP_EALLOC, "Condition: (appCopy == NULL)");
    }

  DPChar *dirCopy = dpStrndup (dir, DEF_DIR_MAX);
  if (dirCopy == NULL)
    {
      dpFree (appCopy);
      dpFree (nameCopy);
      dpFree (*def);

      SET_ERROR_RETURN (DP_EALLOC, "Condition: (dirCopy == NULL)");
    }

  DPSize argsAlloc = argc + 1;
  DPChar **argsCopy = (DPChar **)dpMalloc (argsAlloc * SIZEOF (DPChar *));
  if (argsCopy == NULL)
    {
      dpFree (dirCopy);
      dpFree (appCopy);
      dpFree (nameCopy);
      dpFree (*def);

      SET_ERROR_RETURN (DP_EALLOC, "Condition: (argsCopy == NULL)");
    }
  argsCopy[argc] = NULL;

  i = 0;
  while (i < argc)
    {
      argsCopy[i] = dpStrndup (args[i], DEF_ARG_MAX);
      if (argsCopy[i] == NULL)
        {
          break;
        }

      ++i;
    }

  if (i < argc)
    {
      // Rollback on error
      for (DPByte j = 0; j < i; ++j)
        {
          dpFree (argsCopy[j]);
        }
      dpFree (argsCopy);
      dpFree (dirCopy);
      dpFree (appCopy);
      dpFree (nameCopy);
      dpFree (*def);

      SET_ERROR_RETURN (DP_EALLOC, "Condition: (i < argc)");
    }

  DPSize envsAlloc = envc + 1;
  DPChar **envsCopy = (DPChar **)dpMalloc (envsAlloc * SIZEOF (DPChar *));
  if (envsCopy == NULL)
    {
      for (DPByte j = 0; j < argc; ++j)
        {
          dpFree (argsCopy[j]);
        }
      dpFree (argsCopy);
      dpFree (dirCopy);
      dpFree (appCopy);
      dpFree (nameCopy);
      dpFree (*def);

      SET_ERROR_RETURN (DP_EALLOC, "Condition: (argsCopy == NULL)");
    }
  envsCopy[envc] = NULL;

  k = 0;
  while (k < envc)
    {
      envsCopy[k] = dpStrndup (envs[k], DEF_ENV_MAX);
      if (envsCopy[k] == NULL)
        {
          break;
        }

      ++k;
    }

  if (k < envc)
    {
      // Rollback on error
      for (DPByte j = 0; j < k; ++j)
        {
          dpFree (envsCopy[j]);
        }
      dpFree (envsCopy);
      for (DPByte j = 0; j < argc; ++j)
        {
          dpFree (argsCopy[j]);
        }
      dpFree (argsCopy);
      dpFree (dirCopy);
      dpFree (appCopy);
      dpFree (nameCopy);
      dpFree (*def);

      SET_ERROR_RETURN (DP_EALLOC, "Condition: (i < envc)");
    }

  // Finally, assign the values
  (*def)->app = appCopy;
  (*def)->name = nameCopy;
  (*def)->dir = dirCopy;
  (*def)->argc = argc;
  (*def)->argv = argsCopy;
  (*def)->envc = envc;
  (*def)->envv = envsCopy;

  return DP_OK;
}

DPStatus
dpProcDefClone (DPProcDef *dst, const DPProcDef *src)
{
  CHECK_DEF_RETURN (src);
  CHECK_NULL_RETURN (dst);

  CHECK_STATUS_RETURN (dpProcDefInit (dst, (*src)->name, (*src)->app,
                                      (*src)->dir,
                                      (const DPChar *const *)(*src)->argv,
                                      (const DPChar *const *)(*src)->envv));

  return DP_OK;
}

DPStatus
dpProcDefRefName (const DPProcDef *def, const DPChar **name)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (name);

  *name = (*def)->name;

  return DP_OK;
}

DPStatus
dpProcDefRefApp (const DPProcDef *def, const DPChar **app)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (app);

  *app = (*def)->app;

  return DP_OK;
}

DPStatus
dpProcDefRefDir (const DPProcDef *def, const DPChar **dir)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (dir);

  *dir = (*def)->dir;

  return DP_OK;
}

DPStatus
dpProcDefRefArgv (const DPProcDef *def, const DPChar *const **argv)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (argv);

  *argv = (const DPChar *const *)(*def)->argv;

  return DP_OK;
}

DPStatus
dpProcDefRefEnvs (const DPProcDef *def, const DPChar *const **envs)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (envs);

  *envs = (const DPChar *const *)(*def)->envv;

  return DP_OK;
}

DPStatus
dpProcDefAddArg (DPProcDef *def, const DPChar *arg)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (arg);

  DPSize argLen = dpStrnlen (arg, DEF_ARG_MAX);
  CHECK_RETURN (argLen < DEF_ARG_MAX, DP_ETOOLONG);
  CHECK_RETURN (argLen > 0, DP_EINVALID);

  DPChar *argCopy = dpStrndup (arg, DEF_ARG_MAX);
  CHECK_ALLOC_RETURN (argCopy);

  DPSize argsAlloc = (*def)->argc + 2;
  DPChar **argsCopy
      = (DPChar **)dpRealloc ((*def)->argv, argsAlloc * SIZEOF (DPChar *));
  if (argsCopy == NULL)
    {
      dpFree (argCopy);

      SET_ERROR_RETURN (DP_EALLOC, "Condition: (argsCopy == NULL)");
    }

  argsCopy[argsAlloc - 2] = argCopy;
  argsCopy[argsAlloc - 1] = NULL;

  (*def)->argc = (*def)->argc + 1;
  (*def)->argv = argsCopy;

  return DP_OK;
}

DPStatus
dpProcDefAddEnv (DPProcDef *def, const DPChar *name, const DPChar *val)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (name);
  CHECK_NULL_RETURN (val);

  DPSize nameLen = dpStrnlen (name, DEF_ENV_MAX);
  DPSize valLen = dpStrnlen (val, DEF_ENV_MAX);
  // We are adding extra one byte for '=' sign.
  DPSize envLen = nameLen + valLen + 1;
  CHECK_RETURN (envLen < DEF_ENV_MAX, DP_ETOOLONG);
  CHECK_RETURN (envLen > 0, DP_EINVALID);

  DPSize envAlloc = envLen + 1;
  DPChar *envCopy = (DPChar *)dpCalloc (envAlloc, SIZEOF (DPChar));
  CHECK_NULL_RETURN (envCopy);

  // Concatenate env name, '=', and env value.
  // Note: We have pre-checked the lenght, so the third argument does not
  // really matter here.
  dpStrncat (envCopy, name, DEF_ENV_MAX);
  dpStrncat (envCopy, "=", DEF_ENV_MAX);
  dpStrncat (envCopy, val, DEF_ENV_MAX);

  DPSize envsAlloc = (*def)->envc + 2;
  DPChar **envsCopy
      = (DPChar **)dpRealloc ((*def)->envv, envsAlloc * SIZEOF (DPChar *));
  if (envsCopy == NULL)
    {
      dpFree (envCopy);

      SET_ERROR_RETURN (DP_EALLOC, "Condition: (envsCopy == NULL)");
    }

  envsCopy[envsAlloc - 2] = envCopy;
  envsCopy[envsAlloc - 1] = NULL;

  (*def)->envc = (*def)->envc + 1;
  (*def)->envv = envsCopy;

  return DP_OK;
}

DP_API DPStatus
dpProcDefGetEnvName (const DPProcDef *def, DPChar *name, DPByte idx, DPSize sz,
                     DPSize *used)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (name);

  CHECK_RETURN (idx < (*def)->envc, DP_EBOUND);

  // Just do a slightly modifed strncpy().
  const DPChar *ptr = (*def)->envv[idx];
  DPSize i = 0;
  while (i < sz && ptr[i] != '=' && ptr[i] != '\0')
    {
      name[i] = ptr[i];
      ++i;
    }

  if (i < sz)
    {
      name[i] = '\0';
      ++i;
    }

  if (used != NULL)
    {
      *used = i;
    }

  return DP_OK;
}

DP_API DPStatus
dpProcDefGetEnvVal (const DPProcDef *def, DPChar *name, DPByte idx, DPSize sz,
                    DPSize *used)
{
  CHECK_DEF_RETURN (def);
  CHECK_NULL_RETURN (name);

  CHECK_RETURN (idx < (*def)->envc, DP_EBOUND);

  // Find equal character first that serves as a delimiter.
  DPChar *ptr = (*def)->envv[idx];
  while (*ptr != '=')
    {
      ++ptr;
    }

  // Move one character beyond the delimiter to the start of the value.
  ++ptr;

  // Now, do the standard strncpy().
  DPSize i = 0;
  while (i < sz && ptr[i] != '\0')
    {
      name[i] = ptr[i];
      ++i;
    }

  if (i < sz)
    {
      name[i] = '\0';
      ++i;
    }

  if (used != NULL)
    {
      *used = i;
    }

  return DP_OK;
}
