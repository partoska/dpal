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

#ifndef _CORE_DEF_H_
#define _CORE_DEF_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "core/error.h"
#include "plat/common.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define DEF_NAME_MAX DP_NAME_MAX
#define DEF_APP_MAX DP_APP_MAX
#define DEF_DIR_MAX DP_DIR_MAX
#define DEF_ARG_MAX DP_ARG_MAX
#define DEF_ENV_MAX DP_ENV_MAX

#define CHECK_DEF_RETURN(def)                                                 \
  do                                                                          \
    {                                                                         \
      CHECK_NULL_RETURN (def);                                                \
      CHECK_NULL_RETURN (*def);                                               \
    }                                                                         \
  while (FALSE)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API DPStatus dpProcDefInit (DPProcDef *def, const DPChar *name,
                               const DPChar *app, const DPChar *dir,
                               const DPChar *const args[],
                               const DPChar *const envs[]);
DP_API DPStatus dpProcDefDestroy (DPProcDef *def);
DP_API DPStatus dpProcDefGetName (const DPProcDef *def, DPChar *name,
                                  DPSize sz, DPSize *used);
DP_API DPStatus dpProcDefGetApp (const DPProcDef *def, DPChar *app, DPSize sz,
                                 DPSize *used);
DP_API DPStatus dpProcDefGetDir (const DPProcDef *def, DPChar *dir, DPSize sz,
                                 DPSize *used);
DP_API DPStatus dpProcDefGetArgc (const DPProcDef *def, DPSize *argc);
DP_API DPStatus dpProcDefGetArg (const DPProcDef *def, DPChar *arg, DPByte idx,
                                 DPSize sz, DPSize *used);
DP_API DPStatus dpProcDefGetEnvc (const DPProcDef *def, DPSize *argc);
DP_API DPStatus dpProcDefGetEnv (const DPProcDef *def, DPChar *env, DPByte idx,
                                 DPSize sz, DPSize *used);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API DPStatus dpProcDefInitEx (DPProcDef *def, const DPChar *name,
                                 const DPChar *app, const DPChar *dir,
                                 const DPChar *const args[],
                                 const DPChar *const envs[]);
DP_API DPStatus dpProcDefClone (DPProcDef *dst, const DPProcDef *src);
DP_API DPStatus dpProcDefRefName (const DPProcDef *def, const DPChar **name);
DP_API DPStatus dpProcDefRefApp (const DPProcDef *def, const DPChar **app);
DP_API DPStatus dpProcDefRefDir (const DPProcDef *def, const DPChar **dir);
DP_API DPStatus dpProcDefRefArgv (const DPProcDef *def,
                                  const DPChar *const **argv);
DP_API DPStatus dpProcDefRefEnvs (const DPProcDef *def,
                                  const DPChar *const **envs);
DP_API DPStatus dpProcDefAddArg (DPProcDef *def, const DPChar *arg);
DP_API DPStatus dpProcDefAddEnv (DPProcDef *def, const DPChar *name,
                                 const DPChar *val);
DP_API DPStatus dpProcDefGetEnvName (const DPProcDef *def, DPChar *name,
                                     DPByte idx, DPSize sz, DPSize *used);
DP_API DPStatus dpProcDefGetEnvVal (const DPProcDef *def, DPChar *name,
                                    DPByte idx, DPSize sz, DPSize *used);

#endif
