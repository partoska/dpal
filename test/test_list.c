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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dpal.h"
#include "dpaltypes.h"

#include "list.h"

#define TEST_ASSERT(expr) test_assert ((expr), #expr, __FILE__, __LINE__)

#define OUT_MAX (4096)

static void
test_assert (int result, const char *expr, const char *file, int line)
{
  if (!result)
    {
      printf ("Assertion failed: %s (%s:%d)\n", expr, file, line);
      exit (1);
    }
}

static char outpath[] = "list-XXXXXX";
static int outfd = -1;
static int stdoutfd = -1;
static off_t outoff = 0;

// Redirects stdout (where the list is printed) into the output file.
static void
capture_begin (void)
{
  fflush (stdout);
  TEST_ASSERT (dup2 (outfd, STDOUT_FILENO) == STDOUT_FILENO);
}

static void
capture_end (void)
{
  TEST_ASSERT (dup2 (stdoutfd, STDOUT_FILENO) == STDOUT_FILENO);
}

// Checks everything printed since the last check.
static void
expect (const char *expected)
{
  static char out[OUT_MAX];
  ssize_t n = pread (outfd, out, sizeof (out) - 1, outoff);
  TEST_ASSERT (n >= 0);
  out[n] = '\0';
  outoff += n;

  if (strcmp (out, expected) != 0)
    {
      printf ("Expected:\n%s\nActual:\n%s\n", expected, out);
    }
  TEST_ASSERT (strcmp (out, expected) == 0);
}

static DPCMsgProcInfo
proc (DPId id, const char *name, DPPid pid, DPDword restarts,
      DPProcState state)
{
  DPCMsgProcInfo info
      = { .id = id, .pid = pid, .restarts = restarts, .state = state };
  strncpy (info.name, name, sizeof (info.name));

  return info;
}

// Prints a whole list the way the command executor does.
static void
print_list (DPCFmt fmt, const DPCMsgProcInfo *procs, size_t count)
{
  DPCList list = { .fmt = fmt, .count = 0 };

  capture_begin ();
  dpcListHeader (&list);
  for (size_t i = 0; i < count; ++i)
    {
      TEST_ASSERT (dpcListRow (&procs[i], &list) == 0);
    }
  dpcListFooter (&list);
  capture_end ();

  TEST_ASSERT (list.count == count);
}

static void
test_parse (void)
{
  DPCFmt fmt = DPC_FPLAIN;
  TEST_ASSERT (dpcListParseFmt (&fmt, "json") == 0 && fmt == DPC_FJSON);
  TEST_ASSERT (dpcListParseFmt (&fmt, "csv") == 0 && fmt == DPC_FCSV);
  TEST_ASSERT (dpcListParseFmt (&fmt, "one") == 0 && fmt == DPC_FONE);
  TEST_ASSERT (dpcListParseFmt (&fmt, "plain") == 0 && fmt == DPC_FPLAIN);

  // Unknown formats are refused and leave the format untouched.
  fmt = DPC_FCSV;
  TEST_ASSERT (dpcListParseFmt (&fmt, "xml") < 0);
  TEST_ASSERT (dpcListParseFmt (&fmt, "") < 0);
  TEST_ASSERT (dpcListParseFmt (&fmt, "JSON") < 0);
  TEST_ASSERT (dpcListParseFmt (&fmt, "jsonl") < 0);
  TEST_ASSERT (fmt == DPC_FCSV);
}

static void
test_formats (void)
{
  const DPCMsgProcInfo procs[] = { proc (1, "web", 1441, 2, DP_RUNNING),
                                   proc (4, "worker", -1, 0, DP_STOPPED) };
  const size_t count = sizeof (procs) / sizeof (procs[0]);

  print_list (DPC_FPLAIN, procs, count);
  expect ("   1: web              RUNNING  [   1441 ]        2 ↻\n"
          "   4: worker           STOPPED  [     -1 ]        0 ↻\n");

  print_list (DPC_FJSON, procs, count);
  expect ("[\n"
          "  {\"id\":1,\"name\":\"web\",\"state\":\"running\",\"pid\":1441,"
          "\"restarts\":2},\n"
          "  {\"id\":4,\"name\":\"worker\",\"state\":\"stopped\",\"pid\":-1,"
          "\"restarts\":0}\n"
          "]\n");

  print_list (DPC_FCSV, procs, count);
  expect ("id,name,state,pid,restarts\n"
          "1,\"web\",running,1441,2\n"
          "4,\"worker\",stopped,-1,0\n");

  print_list (DPC_FONE, procs, count);
  expect ("web\n"
          "worker\n");
}

static void
test_empty (void)
{
  // An empty list is still well-formed.
  print_list (DPC_FPLAIN, NULL, 0);
  expect ("");

  print_list (DPC_FJSON, NULL, 0);
  expect ("[\n"
          "]\n");

  print_list (DPC_FCSV, NULL, 0);
  expect ("id,name,state,pid,restarts\n");

  print_list (DPC_FONE, NULL, 0);
  expect ("");

  // A single row has no separator.
  const DPCMsgProcInfo one = proc (7, "db", 99, 1, DP_RUNNING);
  print_list (DPC_FJSON, &one, 1);
  expect ("[\n"
          "  {\"id\":7,\"name\":\"db\",\"state\":\"running\",\"pid\":99,"
          "\"restarts\":1}\n"
          "]\n");
}

static void
test_escaping (void)
{
  // Names loaded from an INI file are not sanitized.
  const DPCMsgProcInfo odd = proc (2, "a\"b\\c\tz", -1, 0, DP_STOPPED);

  print_list (DPC_FJSON, &odd, 1);
  expect ("[\n"
          "  {\"id\":2,\"name\":\"a\\\"b\\\\c\\u0009z\",\"state\":"
          "\"stopped\",\"pid\":-1,\"restarts\":0}\n"
          "]\n");

  print_list (DPC_FCSV, &odd, 1);
  expect ("id,name,state,pid,restarts\n"
          "2,\"a\"\"b\\c\tz\",stopped,-1,0\n");

  print_list (DPC_FONE, &odd, 1);
  expect ("a\"b\\c\tz\n");
}

static void
test_long_name (void)
{
  // A name filling the whole buffer has no terminator, so it must not be
  // read past CMD_NAME_MAX.
  DPCMsgProcInfo full = proc (3, "", -1, 0, DP_STOPPED);
  memset (full.name, 'n', sizeof (full.name));

  char name[CMD_NAME_MAX + 1];
  memset (name, 'n', CMD_NAME_MAX);
  name[CMD_NAME_MAX] = '\0';

  char expected[OUT_MAX];

  print_list (DPC_FONE, &full, 1);
  snprintf (expected, sizeof (expected), "%s\n", name);
  expect (expected);

  print_list (DPC_FCSV, &full, 1);
  snprintf (expected, sizeof (expected),
            "id,name,state,pid,restarts\n3,\"%s\",stopped,-1,0\n", name);
  expect (expected);

  print_list (DPC_FJSON, &full, 1);
  snprintf (expected, sizeof (expected),
            "[\n  {\"id\":3,\"name\":\"%s\",\"state\":\"stopped\","
            "\"pid\":-1,\"restarts\":0}\n]\n",
            name);
  expect (expected);

  // Worst case expansion of every character still fits the buffers.
  memset (full.name, '\x01', sizeof (full.name));
  print_list (DPC_FJSON, &full, 1);

  strcpy (expected, "[\n  {\"id\":3,\"name\":\"");
  for (int i = 0; i < CMD_NAME_MAX; ++i)
    {
      strcat (expected, "\\u0001");
    }
  strcat (expected,
          "\",\"state\":\"stopped\",\"pid\":-1,\"restarts\":0}\n]\n");
  expect (expected);

  memset (full.name, '"', sizeof (full.name));
  print_list (DPC_FCSV, &full, 1);

  strcpy (expected, "id,name,state,pid,restarts\n3,\"");
  for (int i = 0; i < CMD_NAME_MAX; ++i)
    {
      strcat (expected, "\"\"");
    }
  strcat (expected, "\",stopped,-1,0\n");
  expect (expected);
}

int
main (int argc, const char *argv[])
{
  (void)(argc);
  (void)(argv);

  outfd = mkstemp (outpath);
  TEST_ASSERT (outfd >= 0);
  stdoutfd = dup (STDOUT_FILENO);
  TEST_ASSERT (stdoutfd >= 0);

  test_parse ();
  test_formats ();
  test_empty ();
  test_escaping ();
  test_long_name ();

  close (stdoutfd);
  close (outfd);
  remove (outpath);

  return 0;
}
