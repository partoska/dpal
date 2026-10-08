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

#ifndef _CORE_ATTR_H_
#define _CORE_ATTR_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "core/error.h"
#include "plat/common.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define CHECK_ATTR_RETURN(attr)                                               \
  do                                                                          \
    {                                                                         \
      CHECK_NULL_RETURN (attr);                                               \
      CHECK_NULL_RETURN (*attr);                                              \
    }                                                                         \
  while (FALSE)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API DPStatus dpProcAttrInit (DPProcAttr *attr);
DP_API DPStatus dpProcAttrDestroy (DPProcAttr *attr);
DP_API DPStatus dpProcAttrSetMaxMem (DPProcAttr *attr, DPQword mem);
DP_API DPStatus dpProcAttrGetMaxMem (const DPProcAttr *attr, DPQword *mem);
DP_API DPStatus dpProcAttrSetLogMaxSize (DPProcAttr *attr, DPQword sz);
DP_API DPStatus dpProcAttrGetLogMaxSize (const DPProcAttr *attr, DPQword *sz);
DP_API DPStatus dpProcAttrSetLogRot (DPProcAttr *attr, DPByte rot);
DP_API DPStatus dpProcAttrGetLogRot (const DPProcAttr *attr, DPByte *rot);
DP_API DPStatus dpProcAttrSetWatchDir (DPProcAttr *attr, const DPChar *dir);
DP_API DPStatus dpProcAttrGetWatchDir (const DPProcAttr *attr, DPChar *dir,
                                       DPSize sz, DPSize *used);
DP_API DPStatus dpProcAttrSetRestartSec (DPProcAttr *attr, DPDword sec);
DP_API DPStatus dpProcAttrGetRestartSec (const DPProcAttr *attr, DPDword *sec);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API DPStatus dpProcAttrClone (DPProcAttr *dst, const DPProcAttr *src);

#endif
