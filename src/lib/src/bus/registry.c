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

#include "bus/registry.h"
#include "bus/init.h"
#include "core/attr.h"
#include "core/def.h"
#include "core/error.h"
#include "core/info.h"
#include "core/log.h"
#include "plat/alloc.h"
#include "plat/common.h"
#include "plat/logger.h"
#include "plat/str.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define kcalloc(N, Z) dpCalloc (N, Z)
#define kmalloc(Z) dpMalloc (Z)
#define krealloc(P, Z) dpRealloc (P, Z)
#define kfree(P) dpFree (P)
#define kmemset(P, V, Z) dpMemset (P, V, Z)
#define kstrcmp(P, Q) dpStrcmp (P, Q)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct
{
  DPProcDef def;
  DPProcAttr attr;
  DPProcInfoEx info;
  DPLog logout;
  DPLog logerr;
} DPHashMapItem;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// Hash map of registered processes
#include "3rdparty/khash.h"
COMPILE_ASSERT (SIZEOF (khint_t) == SIZEOF (DPId));
KHASH_MAP_INIT_INT (registry, DPHashMapItem *)
static khash_t (registry) * registryMap;
KHASH_MAP_INIT_STR (namebook, DPId)
static khash_t (namebook) * namebookMap;

static const DPId DP_ID_INVALID = -1;

static DPId
findFreeIdMax (void)
{
  DPId max = 0;
  for (khiter_t it = kh_begin (registryMap); it != kh_end (registryMap); ++it)
    {
      if (!kh_exist (registryMap, it))
        {
          continue;
        }

      DPId key = kh_key (registryMap, it);
      if (key > max)
        {
          max = key;
        }
    }

  return max + 1;
}

static DPId
findFreeIdGTE (DPId id)
{
  if (id < 0)
    {
      id = 0;
    }

  while (kh_get (registry, registryMap, id) != kh_end (registryMap))
    {
      ++id;
    }

  return id;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpProcReg (DPId *id, const DPProcDef *def, const DPProcAttr *attr)
{
  CHECK_INIT_RETURN ();
  CHECK_DEF_RETURN (def);

  const DPChar *name;

  // To avoid duplicate checking, reference the name from the provided
  // definition first.
  CHECK_STATUS_RETURN (dpProcDefRefName (def, &name));

  DP_INFO ("Register process: %s", name);

  CHECK_RETURN ((dpProcFind (NULL, name) == DP_BREAK), DP_EEXIST);

  DPProcDef definition;
  CHECK_STATUS_RETURN (dpProcDefClone (&definition, def));

  // Now, let's get name from the cloned definition.
  CHECK_STATUS_RETURN (dpProcDefRefName (&definition, &name));

  DPProcAttr attributes;
  if (attr == NULL)
    {
      CHECK_STATUS_RETURN (dpProcAttrInit (&attributes));
    }
  else
    {
      CHECK_STATUS_RETURN (dpProcAttrClone (&attributes, attr));
    }

  DPProcInfoEx info;
  CHECK_STATUS_RETURN (dpProcInfoExInit (&info));

  DPQword loglim;
  CHECK_STATUS_RETURN (dpProcAttrGetLogMaxSize (&attributes, &loglim));

  DPByte logrot;
  CHECK_STATUS_RETURN (dpProcAttrGetLogRot (&attributes, &logrot));

  DPLog logout;
  CHECK_STATUS_RETURN (
      dpLogInit (&logout, name, DP_LOG_STDOUT, loglim, logrot));

  DPLog logerr;
  CHECK_STATUS_RETURN (
      dpLogInit (&logerr, name, DP_LOG_STDERR, loglim, logrot));

  DPId procId;
  if (id == NULL || *id < 0)
    {
      // If the incoming process id is NULL or invalid, suggest a new id which
      // is maximal.
      procId = findFreeIdMax ();
    }
  else if (kh_get (registry, registryMap, *id) != kh_end (registryMap))
    {
      // If the provided id exists, suggest a new process greater than it.
      procId = findFreeIdGTE (*id);
    }
  else
    {
      // We can utilize provided id since it is free.
      procId = *id;
    }

  DPInt code;
  khiter_t it;
  it = kh_put (namebook, namebookMap, name, &code);
  if (code == -1)
    {
      CHECK_STATUS_RETURN (dpLogDestroy (&logerr));
      CHECK_STATUS_RETURN (dpLogDestroy (&logout));
      CHECK_STATUS_RETURN (dpProcInfoExDestroy (&info));
      CHECK_STATUS_RETURN (dpProcAttrDestroy (&attributes));
      CHECK_STATUS_RETURN (dpProcDefDestroy (&definition));

      SET_ERROR_RETURN (DP_EUNEXPECTED, "Namebook condition: (code == -1)");
    }

  if (code == 0)
    {
      CHECK_STATUS_RETURN (dpLogDestroy (&logerr));
      CHECK_STATUS_RETURN (dpLogDestroy (&logout));
      CHECK_STATUS_RETURN (dpProcInfoExDestroy (&info));
      CHECK_STATUS_RETURN (dpProcAttrDestroy (&attributes));
      CHECK_STATUS_RETURN (dpProcDefDestroy (&definition));

      SET_ERROR_RETURN (DP_EUNEXPECTED, "Namebook condition: (code == 0)");
    }

  if (code == 2 && kh_value (namebookMap, it) != DP_ID_INVALID)
    {
      CHECK_STATUS_RETURN (dpLogDestroy (&logerr));
      CHECK_STATUS_RETURN (dpLogDestroy (&logout));
      CHECK_STATUS_RETURN (dpProcInfoExDestroy (&info));
      CHECK_STATUS_RETURN (dpProcAttrDestroy (&attributes));
      CHECK_STATUS_RETURN (dpProcDefDestroy (&definition));

      SET_ERROR_RETURN (DP_EUNEXPECTED,
                        "Namebook condition: (code == 2 && kh_value "
                        "(namebookMap, it) != DP_ID_INVALID)");
    }

  kh_value (namebookMap, it) = procId;

  it = kh_put (registry, registryMap, procId, &code);
  if (code == -1)
    {
      CHECK_STATUS_RETURN (dpLogDestroy (&logerr));
      CHECK_STATUS_RETURN (dpLogDestroy (&logout));
      CHECK_STATUS_RETURN (dpProcInfoExDestroy (&info));
      CHECK_STATUS_RETURN (dpProcAttrDestroy (&attributes));
      CHECK_STATUS_RETURN (dpProcDefDestroy (&definition));

      SET_ERROR_RETURN (DP_EUNEXPECTED, "Registry condition: (code == -1)");
    }

  if (code == 0)
    {
      CHECK_STATUS_RETURN (dpLogDestroy (&logerr));
      CHECK_STATUS_RETURN (dpLogDestroy (&logout));
      CHECK_STATUS_RETURN (dpProcInfoExDestroy (&info));
      CHECK_STATUS_RETURN (dpProcAttrDestroy (&attributes));
      CHECK_STATUS_RETURN (dpProcDefDestroy (&definition));

      SET_ERROR_RETURN (DP_EUNEXPECTED, "Registry condition: (code == 0)");
    }

  if (code == 2 && kh_value (registryMap, it) != NULL)
    {
      CHECK_STATUS_RETURN (dpLogDestroy (&logerr));
      CHECK_STATUS_RETURN (dpLogDestroy (&logout));
      CHECK_STATUS_RETURN (dpProcInfoExDestroy (&info));
      CHECK_STATUS_RETURN (dpProcAttrDestroy (&attributes));
      CHECK_STATUS_RETURN (dpProcDefDestroy (&definition));

      SET_ERROR_RETURN (
          DP_EUNEXPECTED,
          "Condition: (code == 2 && kh_value (registryMap, it) != NULL)");
    }

  // Key was newly added, we need to allocate item.
  DPHashMapItem *item = (DPHashMapItem *)dpMalloc (SIZEOF (DPHashMapItem));
  if (item == NULL)
    {
      CHECK_STATUS_RETURN (dpLogDestroy (&logerr));
      CHECK_STATUS_RETURN (dpLogDestroy (&logout));
      CHECK_STATUS_RETURN (dpProcInfoExDestroy (&info));
      CHECK_STATUS_RETURN (dpProcAttrDestroy (&attributes));
      CHECK_STATUS_RETURN (dpProcDefDestroy (&definition));

      SET_ERROR_RETURN (DP_EALLOC, "Condition: (item == NULL)");
    }

  item->def = definition;
  item->attr = attributes;
  item->info = info;
  item->logout = logout;
  item->logerr = logerr;
  kh_value (registryMap, it) = item;

  if (id != NULL)
    {
      *id = procId;
    }

  return DP_OK;
}

DPStatus
dpProcExists (DPId id)
{
  CHECK_INIT_RETURN ();

  return (kh_get (registry, registryMap, id) == kh_end (registryMap))
             ? DP_BREAK
             : DP_OK;
}

DPStatus
dpProcFind (DPId *found, const DPChar *name)
{
  CHECK_INIT_RETURN ();

  if (name == NULL)
    {
      return DP_BREAK;
    }

  khiter_t it = kh_get (namebook, namebookMap, name);
  if (it == kh_end (namebookMap))
    {
      return DP_BREAK;
    }

  DPId id = kh_value (namebookMap, it);
  if (id == DP_ID_INVALID)
    {
      return DP_BREAK;
    }

  if (found != NULL)
    {
      *found = id;
    }

  return DP_OK;
}

DPStatus
dpProcIter (DPProcIter *iter)
{
  CHECK_INIT_RETURN ();
  CHECK_NULL_RETURN (iter);

  *iter = DP_ID_INVALID;

  return DP_OK;
}

DPStatus
dpProcIterNext (DPProcIter *iter, DPId *next)
{
  if (!dpIsInitted ())
    {
      return DP_BREAK;
    }

  if (iter == NULL)
    {
      return DP_BREAK;
    }

  DPId id = *iter;
  DPInt dmin = INT_MAX;
  DPId procId = id;
  for (khiter_t it = kh_begin (registryMap); it != kh_end (registryMap); ++it)
    {
      if (!kh_exist (registryMap, it))
        {
          continue;
        }

      // Find the lowest-numbered process larger than the provided one.
      DPId curId = kh_key (registryMap, it);
      if (curId <= id)
        {
          // Lower or equal.
          continue;
        }

      if (curId - id > dmin)
        {
          // Not the lowest-numbered.
          continue;
        }

      dmin = curId - id;
      procId = curId;
    }

  if (next != NULL)
    {
      *next = procId;
    }

  *iter = procId;

  return procId == id ? DP_BREAK : DP_OK;
}

DPStatus
dpProcUnreg (DPId id)
{
  CHECK_INIT_RETURN ();

  DP_INFO ("Unregister process: %d", id);

  const DPProcInfoEx *info = NULL;
  CHECK_STATUS_RETURN (dpProcRefInfoEx (id, &info));

  DPProcStateEx state;
  CHECK_STATUS_RETURN (dpProcInfoExGetState (info, &state));
  switch (state)
    {
    case DP_STOPPED_EX:
    case DP_ERRORED_EX:
      break;

    default:
      SET_ERROR_RETURN (
          DP_ENOT_STOPPED,
          "Condition: (state != DP_STOPPED_EX && state != DP_ERRORED_EX)");
    }

  khiter_t it;
  it = kh_get (registry, registryMap, id);
  CHECK_RETURN (it != kh_end (registryMap), DP_ENOT_FOUND);

  DPHashMapItem *item = kh_value (registryMap, it);
  CHECK_RETURN (item != NULL, DP_EUNEXPECTED);

  kh_value (registryMap, it) = NULL;
  kh_del (registry, registryMap, it);

  const DPChar *name;
  CHECK_STATUS_RETURN (dpProcDefRefName (&item->def, &name));

  it = kh_get (namebook, namebookMap, name);
  CHECK_RETURN (it != kh_end (namebookMap), DP_EUNEXPECTED);
  kh_value (namebookMap, it) = DP_ID_INVALID;
  kh_del (namebook, namebookMap, it);

  CHECK_STATUS_RETURN (dpLogDestroy (&item->logerr));
  CHECK_STATUS_RETURN (dpLogDestroy (&item->logout));
  CHECK_STATUS_RETURN (dpProcInfoExDestroy (&item->info));
  CHECK_STATUS_RETURN (dpProcAttrDestroy (&item->attr));
  CHECK_STATUS_RETURN (dpProcDefDestroy (&item->def));

  dpFree (item);

  return DP_OK;
}

DPStatus
dpProcGetDef (DPProcDef *def, DPId id)
{
  return dpProcGetProps (id, def, NULL, NULL);
}

DPStatus
dpProcGetAttr (DPProcAttr *attr, DPId id)
{
  return dpProcGetProps (id, NULL, attr, NULL);
}

DPStatus
dpProcGetInfo (DPProcInfo *info, DPId id)
{
  return dpProcGetProps (id, NULL, NULL, info);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpRegInit (void)
{
  CHECK_DOUBLE_INIT_RETURN ();

  // Registry map
  registryMap = kh_init (registry);
  CHECK_ALLOC_RETURN (registryMap);

  // Namebook map
  namebookMap = kh_init (namebook);
  CHECK_ALLOC_RETURN (namebookMap);

  return DP_OK;
}

DPStatus
dpRegDestroy (void)
{
  CHECK_INIT_RETURN ();

  // Sanity check that there are no running processes.
  for (khiter_t it = kh_begin (registryMap); it != kh_end (registryMap); ++it)
    {
      if (!kh_exist (registryMap, it))
        {
          continue;
        }

      DPHashMapItem *item = kh_value (registryMap, it);
      if (item == NULL)
        {
          continue;
        }

      DPProcStateEx state;
      CHECK_STATUS_RETURN (dpProcInfoExGetState (&item->info, &state));
      switch (state)
        {
        case DP_STOPPED_EX:
        case DP_ERRORED_EX:
          break;

        default:
          SET_ERROR_RETURN (
              DP_ENOT_STOPPED,
              "Condition: (state != DP_STOPPED_EX && state != DP_ERRORED_EX)");
        }
    }

  // Clean up namebook items.
  for (khiter_t it = kh_begin (namebookMap); it != kh_end (namebookMap); ++it)
    {
      if (!kh_exist (namebookMap, it))
        {
          continue;
        }

      kh_value (namebookMap, it) = DP_ID_INVALID;

      kh_del (namebook, namebookMap, it);
    }

  // Destroy the namebook itself.
  kh_destroy (namebook, namebookMap);

  // Clean up registry items.
  for (khiter_t it = kh_begin (registryMap); it != kh_end (registryMap); ++it)
    {
      if (!kh_exist (registryMap, it))
        {
          continue;
        }

      DPHashMapItem *item = kh_value (registryMap, it);
      if (item == NULL)
        {
          continue;
        }

      CHECK_STATUS_RETURN (dpLogDestroy (&item->logerr));
      CHECK_STATUS_RETURN (dpLogDestroy (&item->logout));
      CHECK_STATUS_RETURN (dpProcInfoExDestroy (&item->info));
      CHECK_STATUS_RETURN (dpProcAttrDestroy (&item->attr));
      CHECK_STATUS_RETURN (dpProcDefDestroy (&item->def));

      kh_value (registryMap, it) = NULL;
      dpFree (item);

      kh_del (registry, registryMap, it);
    }

  // Destroy the registry itself.
  kh_destroy (registry, registryMap);

  return DP_OK;
}

DPStatus
dpProcGetProps (DPId id, DPProcDef *def, DPProcAttr *attr, DPProcInfo *info)
{
  CHECK_INIT_RETURN ();

  khiter_t it = kh_get (registry, registryMap, id);
  CHECK_RETURN (it != kh_end (registryMap), DP_ENOT_FOUND);

  DPHashMapItem *item = kh_value (registryMap, it);
  CHECK_RETURN (item != NULL, DP_ENOT_FOUND);

  if (def != NULL)
    {
      CHECK_STATUS_RETURN (dpProcDefClone (def, &item->def));
    }

  if (attr != NULL)
    {
      CHECK_STATUS_RETURN (dpProcAttrClone (attr, &item->attr));
    }

  if (info != NULL)
    {
      CHECK_STATUS_RETURN (dpProcInfoInfer (info, &item->info));
    }

  return DP_OK;
}

DPStatus
dpProcPtrInfoEx (DPId id, DPProcInfoEx **info)
{
  CHECK_INIT_RETURN ();
  CHECK_NULL_RETURN (info);

  *info = NULL;

  khiter_t it = kh_get (registry, registryMap, id);
  CHECK_RETURN (it != kh_end (registryMap), DP_ENOT_FOUND);

  DPHashMapItem *item = kh_value (registryMap, it);
  CHECK_RETURN (item != NULL, DP_ENOT_FOUND);

  *info = &item->info;

  return DP_OK;
}

DPStatus
dpProcRefInfoEx (DPId id, const DPProcInfoEx **info)
{
  CHECK_INIT_RETURN ();

  DPProcInfoEx *aux = NULL;
  CHECK_STATUS_RETURN (dpProcPtrInfoEx (id, &aux));

  if (info != NULL)
    {
      *info = aux;
    }

  return DP_OK;
}

DPStatus
dpProcPtrLog (DPId id, DPLog **out, DPLog **err)
{
  CHECK_INIT_RETURN ();

  if (out != NULL)
    {
      *out = NULL;
    }

  if (err != NULL)
    {
      *err = NULL;
    }

  khiter_t it = kh_get (registry, registryMap, id);
  CHECK_RETURN (it != kh_end (registryMap), DP_ENOT_FOUND);

  DPHashMapItem *item = kh_value (registryMap, it);
  CHECK_RETURN (item != NULL, DP_ENOT_FOUND);

  if (out != NULL)
    {
      *out = &item->logout;
    }

  if (err != NULL)
    {
      *err = &item->logerr;
    }

  return DP_OK;
}

DPStatus
dpProcRefLog (DPId id, const DPLog **out, const DPLog **err)
{
  DPLog *auxout = NULL;
  DPLog *auxerr = NULL;
  CHECK_STATUS_RETURN (dpProcPtrLog (id, &auxout, &auxerr));

  if (out != NULL)
    {
      *out = auxout;
    }

  if (err != NULL)
    {
      *err = auxerr;
    }

  return DP_OK;
}

DPStatus
dpProcRefProps (DPId id, const DPProcDef **def, const DPProcAttr **attr,
                const DPProcInfoEx **info)
{
  CHECK_INIT_RETURN ();

  if (def != NULL)
    {
      *def = NULL;
    }

  if (attr != NULL)
    {
      *attr = NULL;
    }

  if (info != NULL)
    {
      *info = NULL;
    }

  khiter_t it = kh_get (registry, registryMap, id);
  CHECK_RETURN (it != kh_end (registryMap), DP_ENOT_FOUND);

  DPHashMapItem *item = kh_value (registryMap, it);
  CHECK_RETURN (item != NULL, DP_ENOT_FOUND);

  if (def != NULL)
    {
      *def = &item->def;
    }

  if (attr != NULL)
    {
      *attr = &item->attr;
    }

  if (info != NULL)
    {
      *info = &item->info;
    }

  return DP_OK;
}

DPStatus
dpRegEach (DPRegVisitor visitor, DPOnVisitErrorCb cb)
{
  CHECK_INIT_RETURN ();
  CHECK_NULL_RETURN (visitor);

  for (khiter_t it = kh_begin (registryMap); it != kh_end (registryMap); ++it)
    {
      if (!kh_exist (registryMap, it))
        {
          continue;
        }

      DPId key = kh_key (registryMap, it);
      if (key == DP_ID_INVALID)
        {
          continue;
        }

      DPStatus status = visitor (key);
      if (status == DP_OK)
        {
          continue;
        }

      if (status == DP_BREAK)
        {
          break;
        }

      // Visit ended up with an error
      DPStatus cbStatus = cb (status);
      CHECK_RETURN (cbStatus == DP_OK || cbStatus == DP_BREAK, DP_EUNEXPECTED);
      if (cbStatus == DP_BREAK)
        {
          return status;
        }
    }

  return DP_OK;
}

DPStatus
dpVisitContinueCb (DPStatus error)
{
  UNUSED (error);

  // Just print the error and continue ...
  dpPrintError (NULL);

  return DP_OK;
}

DPStatus
dpVisitBreakCb (DPStatus error)
{
  UNUSED (error);

  // Just break and propagate the error further ...
  return DP_BREAK;
}
