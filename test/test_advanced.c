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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dpal.h"
#include "dpaltypes.h"

#define TEST_ASSERT(expr) test_assert ((expr), #expr, __FILE__, __LINE__)

static void
test_assert (int result, const char *expr, const char *file, int line)
{
  if (!result)
    {
      printf ("Assertion failed: %s (%s:%d)\n", expr, file, line);
      exit (1);
    }
}

static void
print_error (const char *context)
{
  char err[256];
  if (dpGetErrorStr (err, sizeof (err), NULL) == DP_OK)
    {
      printf ("%s failed: %s\n", context, err);
    }
  else
    {
      printf ("%s failed\n", context);
    }
}

int
main (int argc, const char *argv[])
{
  (void)(argc);
  (void)(argv);

  if (dpInit () != DP_OK)
    {
      print_error ("dpInit");
      return 1;
    }

  remove ("log/echo-out.log");
  remove ("log/echo-err.log");
  remove ("log/pwd-out.log");
  remove ("log/pwd-err.log");

  const char *proc1Args[] = { "echo", "success", NULL };
  const char *proc1Envs[] = { NULL };
  DPProcDef proc1Def;
  if (dpProcDefInit (&proc1Def, "echo", "echo", ".", proc1Args, proc1Envs)
      != DP_OK)
    {
      print_error ("dpProcDefInit");
      return 1;
    }

  char aux[64];
  if (dpProcDefGetApp (&proc1Def, aux, 64, NULL) != DP_OK)
    {
      print_error ("dpProcDefGetApp");
      return 1;
    }
  TEST_ASSERT (strcmp (aux, "echo") == 0);

  const char *proc2Args[] = { "pwd", NULL };
  const char *proc2Envs[] = { NULL };
  DPProcDef proc2Def;
  if (dpProcDefInit (&proc2Def, "pwd", "pwd", ".", proc2Args, proc2Envs)
      != DP_OK)
    {
      print_error ("dpProcDefInit");
      return 1;
    }

  if (dpProcDefGetApp (&proc2Def, aux, 64, NULL) != DP_OK)
    {
      print_error ("dpProcDefGetApp");
      return 1;
    }
  TEST_ASSERT (strcmp (aux, "pwd") == 0);

  DPSize n;
  if (dpProcDefGetArgc (&proc2Def, &n) != DP_OK)
    {
      print_error ("dpProcDefGetArgc");
      return 1;
    }
  TEST_ASSERT (n == 1);

  if (dpProcDefGetDir (&proc1Def, aux, 64, NULL) != DP_OK)
    {
      print_error ("dpProcDefGetDir");
      return 1;
    }
  TEST_ASSERT (strcmp (aux, ".") == 0);

  if (dpProcDefGetArgc (&proc1Def, &n) != DP_OK)
    {
      print_error ("dpProcDefGetArgc");
      return 1;
    }
  TEST_ASSERT (n == 2);

  if (dpProcDefGetArg (&proc1Def, aux, 0, 64, &n) != DP_OK)
    {
      print_error ("dpProcDefGetArg");
      return 1;
    }
  TEST_ASSERT (strcmp (aux, "echo") == 0 && n == 5);

  if (dpProcDefGetArg (&proc1Def, aux, 1, 64, &n) != DP_OK)
    {
      print_error ("dpProcDefGetArg");
      return 1;
    }
  TEST_ASSERT (strcmp (aux, "success") == 0 && n == 8);

  DPProcAttr attr;
  if (dpProcAttrInit (&attr) != DP_OK)
    {
      print_error ("dpProcAttrInit");
      return 1;
    }
  if (dpProcAttrSetLogMaxSize (&attr, 2 * 1024 * 1024) != DP_OK)
    {
      print_error ("dpProcAttrSetLogMaxSize");
      return 1;
    }

  DPId id1 = 0;
  if (dpProcReg (&id1, &proc1Def, &attr) != DP_OK)
    {
      print_error ("dpProcReg");
      return 1;
    }
  TEST_ASSERT (id1 >= 0);

  DPId tmp;
  if (dpProcFind (&tmp, "echo") != DP_OK)
    {
      return 1;
    }

  TEST_ASSERT (tmp == id1);

  DPId id2 = 0;
  if (dpProcReg (&id2, &proc2Def, &attr) != DP_OK)
    {
      print_error ("dpProcReg");
      return 1;
    }
  TEST_ASSERT (id2 >= 0);
  TEST_ASSERT (id2 != id1);
  TEST_ASSERT (id2 > id1);

  if (dpProcUnreg (id1) != DP_OK)
    {
      print_error ("dpProcUnreg");
      return 1;
    }

  DPId old_id1 = id1;
  if (dpProcReg (&id1, &proc1Def, &attr) != DP_OK)
    {
      print_error ("dpProcReg");
      return 1;
    }
  TEST_ASSERT (id1 >= 0);
  TEST_ASSERT (id1 == old_id1);

  if (dpProcStart (id1) != DP_OK)
    {
      print_error ("dpProcStart");
      return 1;
    }

  if (dpProcStart (id2) != DP_OK)
    {
      print_error ("dpProcStart");
      return 1;
    }

  if (dpTick () != DP_OK)
    {
      print_error ("dpTick");
      return 1;
    }

  sleep (1);

  if (dpProcRestart (id1) != DP_OK)
    {
      print_error ("dpProcReg");
      return 1;
    }

  if (dpTick () != DP_OK)
    {
      print_error ("dpTick");
      return 1;
    }

  sleep (1);

  if (dpProcStop (id1) != DP_OK)
    {
      print_error ("dpProcStop");
      return 1;
    }

  if (dpProcStop (id2) != DP_OK)
    {
      print_error ("dpProcStop");
      return 1;
    }

  if (dpTick () != DP_OK)
    {
      print_error ("dpTick");
      return 1;
    }

  sleep (1);

  if (dpProcAttrDestroy (&attr) != DP_OK)
    {
      print_error ("dpProcAttrDestroy");
      return 1;
    }

  if (dpProcDefDestroy (&proc1Def) != DP_OK)
    {
      print_error ("dpProcDefDestroy");
      return 1;
    }

  if (dpProcDefDestroy (&proc2Def) != DP_OK)
    {
      print_error ("dpProcDefDestroy");
      return 1;
    }

  if (dpDestroy () != DP_OK)
    {
      print_error ("dpDestroy");
      return 1;
    }

  if (dpTick () == DP_OK)
    {
      printf ("dpTick: Passed after dpDestroy!\n");
      return 1;
    }

  FILE *logFile = fopen ("log/echo-out.log", "r");
  if (logFile == NULL)
    {
      printf ("Failed to open log/echo-out.log\n");
      return 1;
    }

  char buffer[256];
  int found = 0;
  while (fgets (buffer, sizeof (buffer), logFile) != NULL)
    {
      if (strstr (buffer, "success") != NULL)
        {
          found = 1;
          break;
        }
    }

  fclose (logFile);

  if (!found)
    {
      printf ("String 'success' not found in log/echo-out.log\n");
      return 1;
    }

  return 0;
}
