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

#ifndef _PLAT_FORK_H_
#define _PLAT_FORK_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dpaltypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API const DPInt DP_WNOHANG;
DP_API const DPInt DP_SIGTERM;

DP_API DPPid dpFork (void);
DP_API DPInt dpExecvp (const DPChar *app, DPChar *const args[]);
DP_API void dpExit (DPInt status);

DP_API DPPid dpWaitpid (DPPid pid, DPInt *status, DPInt options);
DP_API DPBool DP_WIFEXITED (DPInt status);
DP_API DPInt DP_WEXITSTATUS (DPInt status);
DP_API DPBool DP_WIFSIGNALED (DPInt status);
DP_API DPInt DP_WTERMSIG (DPInt status);

DP_API DPInt dpKill (DPPid pid, DPInt sig);

#endif
