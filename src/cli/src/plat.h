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

#ifndef _PLAT_H_
#define _PLAT_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dpaltypes.h"

#include "plat/common.h"
#include "plat/types.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define DPCNoArgument 0
#define DPCRequiredArgument 1
#define DPCOptionalArgument 2

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct DPCOption
{
  const DPChar *name;
  DPInt has_arg;
  DPInt *flag;
  DPInt val;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// ===============================================================
// Alloc
// ===============================================================

DP_API void dpcFree (void *ptr);

// ===============================================================
// Strings
// ===============================================================

DP_API void *dpcMemcpy (void *restrict dst, const void *restrict src,
                        DPSize n);
DP_API DPSize dpcStrlen (const DPChar *src);
DP_API DPSize dpcStrnlen (const DPChar *src, DPSize max);
DP_API DPChar *dpcStrncpy (DPChar *dst, const DPChar *src, DPSize n);
DP_API DPChar *dpcStrndup (const DPChar *src, DPSize n);
DP_API DPChar *dpcStrncat (DPChar *dst, const DPChar *src, DPSize n);
DP_API DPInt dpcStrcmp (const DPChar *src1, const DPChar *src2);
DP_API DPInt dpcAtoi (const DPChar *num);
DP_API DPChar *dpcBasename (DPChar *path);
DP_API DPChar *dpcItoa10 (DPInt num, DPChar *str);

// ===============================================================
// Program flow
// ===============================================================

DP_API void dpcExit (DPInt status);
DP_API DPDword dpcSleep (DPDword sec);
DP_API DPInt dpcSigIntHandler (void (*handler) (DPInt));
DP_API DPInt dpcSigTermHandler (void (*handler) (DPInt));

// ===============================================================
// File system
// ===============================================================

DP_API DPChar *dpcGetcwd (DPChar *dir, DPSize n);
DP_API DPInt dpcChdir (const DPChar *path);
DP_API DPBool dpcDirExists (const DPChar *dir);
DP_API DPBool dpcFileExists (const DPChar *file);
DP_API DPInt dpcMkdir (const DPChar *path);
DP_API DPInt dpcUnlink (const DPChar *path);
DP_API DPChar *dpcRealpath (const DPChar *restrict path,
                            DPChar *restrict resolved);
DP_API DPChar *dpcRealpathne (const DPChar *path);

// ===============================================================
// Environment
// ===============================================================

DP_API DPChar *dpcGetenv (const DPChar *name);
DP_API DPInt dpcPutenv (DPChar *env);

// ===============================================================
// Command line options
// ===============================================================

DP_API DPInt dpcGetoptLong (DPInt argc, DPChar *argv[],
                            const DPChar *optstring,
                            const struct DPCOption *longopts,
                            DPInt *longindex);
DP_API DPChar *dpcOptArg (void);
DP_API DPInt dpcOptInd (void);

// ===============================================================
// Messages
// ===============================================================

DPInt dpcFtok (const DPChar *pathname, DPInt projId);
DPInt dpcMsgGet (DPInt key, DPBool create, DPBool excl);
DPInt dpcMsgSnd (DPInt msgid, const void *msgp, DPSize msgsz);
DPSSize dpcMsgRcv (DPInt msgid, void *msgp, DPSize msgsz, DPLong msgtyp);
DPInt dpcMsgRm (DPInt msgid);

#endif
