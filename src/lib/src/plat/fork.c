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

#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "plat/fork.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

const DPInt DP_WNOHANG = WNOHANG;

const DPInt DP_SIGTERM = SIGTERM;

DPPid
dpFork (void)
{
  return fork ();
}

DPInt
dpExecvp (const DPChar *app, DPChar *const args[])
{
  return execvp (app, args);
}

void
dpExit (DPInt status)
{
  exit (status);
}

DPPid
dpWaitpid (DPPid pid, DPInt *status, DPInt options)
{
  return waitpid (pid, status, options);
}

DPBool
DP_WIFEXITED (DPInt status)
{
  return WIFEXITED (status);
}

DPInt
DP_WEXITSTATUS (DPInt status)
{
  return WEXITSTATUS (status);
}

DPBool
DP_WIFSIGNALED (DPInt status)
{
  return WIFSIGNALED (status);
}

DPInt
DP_WTERMSIG (DPInt status)
{
  return WTERMSIG (status);
}

DPInt
dpKill (DPPid pid, DPInt sig)
{
  return kill (pid, sig);
}
