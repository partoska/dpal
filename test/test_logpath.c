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

static int
file_contains (const char *path, const char *text)
{
  FILE *file = fopen (path, "r");
  if (file == NULL)
    {
      return 0;
    }

  char buffer[256];
  int found = 0;
  while (fgets (buffer, sizeof (buffer), file) != NULL)
    {
      if (strstr (buffer, text) != NULL)
        {
          found = 1;
          break;
        }
    }

  fclose (file);

  return found;
}

static void
test_paths (void)
{
  char path[DP_LOG_PATH_MAX];
  DPSize used = 0;

  // Works without the library being initialized.
  TEST_ASSERT (dpLogPath ("web", DP_LOG_STDOUT, path, sizeof (path), &used)
               == DP_OK);
  TEST_ASSERT (strcmp (path, "log/web-out.log") == 0);
  TEST_ASSERT (used == strlen ("log/web-out.log") + 1);

  TEST_ASSERT (dpLogPath ("web", DP_LOG_STDERR, path, sizeof (path), NULL)
               == DP_OK);
  TEST_ASSERT (strcmp (path, "log/web-err.log") == 0);

  TEST_ASSERT (dpLogPath ("Web_2-x", DP_LOG_STDOUT, path, sizeof (path), NULL)
               == DP_OK);
  TEST_ASSERT (strcmp (path, "log/Web_2-x-out.log") == 0);

  // The buffer must fit the whole path including the terminator.
  char exact[16];
  TEST_ASSERT (dpLogPath ("web", DP_LOG_STDOUT, exact, sizeof (exact), &used)
               == DP_OK);
  TEST_ASSERT (strcmp (exact, "log/web-out.log") == 0);

  char small[15];
  used = 0;
  TEST_ASSERT (dpLogPath ("web", DP_LOG_STDOUT, small, sizeof (small), &used)
               == DP_ETOOLONG);
  TEST_ASSERT (used == sizeof (exact));

  // The longest possible name fits into DP_LOG_PATH_MAX.
  char name[DP_NAME_MAX + 1];
  memset (name, 'a', sizeof (name));
  name[DP_NAME_MAX - 1] = '\0';
  TEST_ASSERT (dpLogPath (name, DP_LOG_STDERR, path, sizeof (path), &used)
               == DP_OK);
  TEST_ASSERT (used <= DP_LOG_PATH_MAX);
  TEST_ASSERT (used == strlen (path) + 1);

  name[DP_NAME_MAX - 1] = 'a';
  name[DP_NAME_MAX] = '\0';
  TEST_ASSERT (dpLogPath (name, DP_LOG_STDOUT, path, sizeof (path), NULL)
               == DP_ETOOLONG);

  // Invalid arguments.
  TEST_ASSERT (dpLogPath (NULL, DP_LOG_STDOUT, path, sizeof (path), NULL)
               == DP_ENULL);
  TEST_ASSERT (dpLogPath ("web", DP_LOG_STDOUT, NULL, sizeof (path), NULL)
               == DP_ENULL);
  TEST_ASSERT (dpLogPath ("web", (DPLogType)0, path, sizeof (path), NULL)
               == DP_EINVALID);
  TEST_ASSERT (dpLogPath ("web", (DPLogType)3, path, sizeof (path), NULL)
               == DP_EINVALID);

  // Names which are not valid process names cannot escape the log directory.
  const char *invalid[] = { "", "../web", "a/b", "1web", "-web", "we b", NULL };
  for (int i = 0; invalid[i] != NULL; ++i)
    {
      TEST_ASSERT (dpLogPath (invalid[i], DP_LOG_STDOUT, path, sizeof (path),
                              NULL)
                   == DP_EINVALID);
    }
}

static void
test_process_logs (void)
{
  char outpath[DP_LOG_PATH_MAX];
  char errpath[DP_LOG_PATH_MAX];
  TEST_ASSERT (
      dpLogPath ("logpath", DP_LOG_STDOUT, outpath, sizeof (outpath), NULL)
      == DP_OK);
  TEST_ASSERT (
      dpLogPath ("logpath", DP_LOG_STDERR, errpath, sizeof (errpath), NULL)
      == DP_OK);

  remove (outpath);
  remove (errpath);

  TEST_ASSERT (dpInit () == DP_OK);

  const char *args[] = { "sh", "-c", "echo out-line; echo err-line 1>&2",
                         NULL };
  const char *envs[] = { NULL };
  DPProcDef def;
  TEST_ASSERT (dpProcDefInit (&def, "logpath", "sh", ".", args, envs)
               == DP_OK);

  DPProcAttr attr;
  TEST_ASSERT (dpProcAttrInit (&attr) == DP_OK);

  DPId id = 0;
  TEST_ASSERT (dpProcReg (&id, &def, &attr) == DP_OK);
  TEST_ASSERT (dpProcAttrDestroy (&attr) == DP_OK);
  TEST_ASSERT (dpProcDefDestroy (&def) == DP_OK);

  TEST_ASSERT (dpProcStart (id) == DP_OK);
  TEST_ASSERT (dpTick () == DP_OK);

  sleep (1);

  TEST_ASSERT (dpTick () == DP_OK);
  TEST_ASSERT (dpProcStop (id) == DP_OK);
  TEST_ASSERT (dpTick () == DP_OK);

  sleep (1);

  TEST_ASSERT (dpDestroy () == DP_OK);

  // The process output is captured in the files the paths point to.
  TEST_ASSERT (file_contains (outpath, "out-line"));
  TEST_ASSERT (file_contains (errpath, "err-line"));
  TEST_ASSERT (!file_contains (outpath, "err-line"));
}

int
main (int argc, const char *argv[])
{
  (void)(argc);
  (void)(argv);

  test_paths ();
  test_process_logs ();

  return 0;
}
