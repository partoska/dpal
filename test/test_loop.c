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

#include <unistd.h>

#include "dpal.h"

static int
control_loop (void)
{
  int tick_count = 3;
  while (tick_count > 0)
    {
      if (dpTick () != DP_OK)
        {
          return 1;
        }

      sleep (1);
      --tick_count;
    }

  return 0;
}

int
main (int argc, const char *argv[])
{
  (void)(argc);
  (void)(argv);

  if (dpInit () != DP_OK)
    {
      return 1;
    }

  const char *echoArgs[] = { "echo", "Control loop test", NULL };
  const char *echoEnvs[] = { NULL };
  DPProcDef procDef;
  if (dpProcDefInit (&procDef, "echo-loop", "echo", ".", echoArgs, echoEnvs)
      != DP_OK)
    {
      return 1;
    }

  DPProcAttr attr;
  if (dpProcAttrInit (&attr) != DP_OK)
    {
      return 1;
    }

  DPId id = 0;
  if (dpProcReg (&id, &procDef, &attr) != DP_OK)
    {
      return 1;
    }

  if (dpProcAttrDestroy (&attr) != DP_OK)
    {
      return 1;
    }

  if (dpProcDefDestroy (&procDef) != DP_OK)
    {
      return 1;
    }

  if (dpProcStart (id) != DP_OK)
    {
      return 1;
    }

  if (control_loop () != 0)
    {
      return 1;
    }

  if (dpProcStop (id) != DP_OK)
    {
      return 1;
    }

  if (dpTick () != DP_OK)
    {
      return 1;
    }

  sleep (1);

  if (dpDestroy () != DP_OK)
    {
      return 1;
    }

  return 0;
}
