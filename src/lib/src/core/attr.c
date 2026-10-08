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

#include "core/attr.h"
#include "core/error.h"
#include "plat/alloc.h"
#include "plat/common.h"
#include "plat/str.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define WATCH_DIR_MAX 1024

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct DPSProcAttr
{
  DPQword memlim;
  DPQword loglim;
  DPByte logrot;
  DPDword restartsec;
  DPChar *watchdir;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPStatus
dpProcAttrInit (DPProcAttr *attr)
{
  CHECK_NULL_RETURN (attr);

  *attr = (DPProcAttr)dpMalloc (SIZEOF (struct DPSProcAttr));
  CHECK_ALLOC_RETURN (*attr);

  // Apply defaults.
  (*attr)->memlim = 0;
  (*attr)->loglim = 0;
  (*attr)->logrot = 1;
  (*attr)->restartsec = 0;
  (*attr)->watchdir = NULL;

  return DP_OK;
}

DPStatus
dpProcAttrDestroy (DPProcAttr *attr)
{
  CHECK_ATTR_RETURN (attr);

  if ((*attr)->watchdir != NULL)
    {
      dpFree ((*attr)->watchdir);
    }

  dpFree (*attr);

  *attr = NULL;

  return DP_OK;
}

DPStatus
dpProcAttrSetMaxMem (DPProcAttr *attr, DPQword mem)
{
  CHECK_ATTR_RETURN (attr);

  (*attr)->memlim = mem;

  return DP_OK;
}

DPStatus
dpProcAttrGetMaxMem (const DPProcAttr *attr, DPQword *mem)
{
  CHECK_ATTR_RETURN (attr);
  CHECK_NULL_RETURN (mem);

  *mem = (*attr)->memlim;

  return DP_OK;
}

DPStatus
dpProcAttrSetLogMaxSize (DPProcAttr *attr, DPQword sz)
{
  CHECK_ATTR_RETURN (attr);

  (*attr)->loglim = sz;

  return DP_OK;
}

DPStatus
dpProcAttrGetLogMaxSize (const DPProcAttr *attr, DPQword *sz)
{
  CHECK_ATTR_RETURN (attr);
  CHECK_NULL_RETURN (sz);

  *sz = (*attr)->loglim;

  return DP_OK;
}

DPStatus
dpProcAttrSetLogRot (DPProcAttr *attr, DPByte rot)
{
  CHECK_ATTR_RETURN (attr);

  (*attr)->logrot = rot;

  return DP_OK;
}

DPStatus
dpProcAttrGetLogRot (const DPProcAttr *attr, DPByte *rot)
{
  CHECK_ATTR_RETURN (attr);
  CHECK_NULL_RETURN (rot);

  *rot = (*attr)->logrot;

  return DP_OK;
}

DPStatus
dpProcAttrSetRestartSec (DPProcAttr *attr, DPDword sec)
{
  CHECK_ATTR_RETURN (attr);

  (*attr)->restartsec = sec;

  return DP_OK;
}

DPStatus
dpProcAttrGetRestartSec (const DPProcAttr *attr, DPDword *sec)
{
  CHECK_ATTR_RETURN (attr);
  CHECK_NULL_RETURN (sec);

  *sec = (*attr)->restartsec;

  return DP_OK;
}

DPStatus
dpProcAttrClone (DPProcAttr *dst, const DPProcAttr *src)
{
  CHECK_ATTR_RETURN (src);
  CHECK_NULL_RETURN (dst);

  CHECK_STATUS_RETURN (dpProcAttrInit (dst));

  (*dst)->memlim = (*src)->memlim;
  (*dst)->loglim = (*src)->loglim;
  (*dst)->logrot = (*src)->logrot;
  (*dst)->restartsec = (*src)->restartsec;

  if ((*src)->watchdir != NULL)
    {
      DPChar *watchdirCopy = dpStrndup ((*src)->watchdir, WATCH_DIR_MAX);
      if (watchdirCopy == NULL)
        {
          dpProcAttrDestroy (dst);

          SET_ERROR_RETURN (DP_EALLOC, "Condition: (watchdirCopy == NULL)");
        }

      (*dst)->watchdir = watchdirCopy;
    }

  return DP_OK;
}
