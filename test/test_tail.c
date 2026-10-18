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

#include "tail.h"

#define TEST_ASSERT(expr) test_assert ((expr), #expr, __FILE__, __LINE__)

#define OUT_MAX (65536)

static void
test_assert (int result, const char *expr, const char *file, int line)
{
  if (!result)
    {
      printf ("Assertion failed: %s (%s:%d)\n", expr, file, line);
      exit (1);
    }
}

static char workdir[] = "tail-XXXXXX";
static int outfd = -1;
static off_t outoff = 0;

static const char *
log_path (const char *name, DPLogType type)
{
  static char path[256];
  char rel[DP_LOG_PATH_MAX];
  TEST_ASSERT (dpLogPath (name, type, rel, sizeof (rel), NULL) == DP_OK);
  snprintf (path, sizeof (path), "%s/%s", workdir, rel);

  return path;
}

static void
log_append (const char *name, DPLogType type, const char *data)
{
  FILE *file = fopen (log_path (name, type), "a");
  TEST_ASSERT (file != NULL);
  TEST_ASSERT (fputs (data, file) >= 0);
  fclose (file);
}

static void
log_remove (const char *name, DPLogType type)
{
  remove (log_path (name, type));
}

static void
rot_path (const char *name, int idx, char *path, size_t sz)
{
  snprintf (path, sz, "%s.%d", log_path (name, DP_LOG_STDOUT), idx);
}

static void
rot_write (const char *name, int idx, const char *data)
{
  char path[512];
  rot_path (name, idx, path, sizeof (path));
  FILE *file = fopen (path, "w");
  TEST_ASSERT (file != NULL);
  TEST_ASSERT (fputs (data, file) >= 0);
  fclose (file);
}

// Rotates the stdout log the way the library does: drops the oldest file,
// shifts the others, and starts a new current one.
static void
log_rotate (const char *name, int rot)
{
  char older[512];
  char newer[512];

  rot_path (name, rot - 1, older, sizeof (older));
  remove (older);

  for (int i = rot - 1; i > 0; --i)
    {
      rot_path (name, i, older, sizeof (older));
      rot_path (name, i - 1, newer, sizeof (newer));
      rename (newer, older);
    }

  rot_path (name, 0, older, sizeof (older));
  snprintf (newer, sizeof (newer), "%s", log_path (name, DP_LOG_STDOUT));
  TEST_ASSERT (rename (newer, older) == 0);
  log_append (name, DP_LOG_STDOUT, "");
}

static void
log_remove_all (const char *name)
{
  char path[512];
  for (int i = 0; i < 4; ++i)
    {
      rot_path (name, i, path, sizeof (path));
      remove (path);
    }

  log_remove (name, DP_LOG_STDOUT);
  log_remove (name, DP_LOG_STDERR);
}

// Checks everything the follower printed since the last check.
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

static void
test_follow (void)
{
  log_append ("web", DP_LOG_STDOUT, "l1\nl2\nl3\n");

  DPCTail tail = NULL;
  TEST_ASSERT (dpcTailInit (&tail, workdir, outfd) == 0);
  TEST_ASSERT (dpcTailAdd (&tail, "web") == 0);
  TEST_ASSERT (dpcTailAdd (&tail, "worker") == 0);

  // Duplicates are ignored and invalid names refused.
  TEST_ASSERT (dpcTailAdd (&tail, "web") == 0);
  TEST_ASSERT (dpcTailAdd (&tail, "../web") < 0);

  // Backlog of the last lines, prefixes padded to the longest name.
  TEST_ASSERT (dpcTailStart (&tail, 2) == 0);
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("web    | l2\n"
          "web    | l3\n");

  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("");

  // A log file appearing later is followed from its beginning.
  log_append ("web", DP_LOG_STDERR, "boom\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("web    ! boom\n");

  // Multiple processes.
  log_append ("worker", DP_LOG_STDOUT, "w1\n");
  log_append ("web", DP_LOG_STDOUT, "x\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("web    | x\n"
          "worker | w1\n");

  // Incomplete lines are joined.
  log_append ("web", DP_LOG_STDOUT, "par");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("");
  log_append ("web", DP_LOG_STDOUT, "tial\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("web    | partial\n");

  // An incomplete line is printed once no more data arrives, and its
  // terminating newline does not produce an empty line.
  log_append ("web", DP_LOG_STDOUT, "prompt> ");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("web    | prompt> \n");
  log_append ("web", DP_LOG_STDOUT, "\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("");
  log_append ("web", DP_LOG_STDOUT, "next\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("web    | next\n");

  // Rotation splitting a line: the old file is drained, the line continues
  // in the new file.
  char rotated[512];
  snprintf (rotated, sizeof (rotated), "%s.0",
            log_path ("web", DP_LOG_STDOUT));
  log_append ("web", DP_LOG_STDOUT, "before\nspl");
  TEST_ASSERT (rename (log_path ("web", DP_LOG_STDOUT), rotated) == 0);
  log_append ("web", DP_LOG_STDOUT, "it\nafter\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("web    | before\n"
          "web    | split\n"
          "web    | after\n");

  // Removed and re-created.
  log_remove ("web", DP_LOG_STDOUT);
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("");
  log_append ("web", DP_LOG_STDOUT, "fresh\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("web    | fresh\n");

  // Truncated in place.
  TEST_ASSERT (truncate (log_path ("web", DP_LOG_STDOUT), 0) == 0);
  log_append ("web", DP_LOG_STDOUT, "re\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("web    | re\n");

  // Long lines are split, a line of exactly the maximum length is not
  // followed by an empty one.
  static char data[2 * TAIL_LINE_MAX];
  static char expected[4 * TAIL_LINE_MAX];
  memset (data, 'x', TAIL_LINE_MAX + 100);
  strcpy (data + TAIL_LINE_MAX + 100, "\n");
  log_append ("web", DP_LOG_STDOUT, data);
  memset (data, 'y', TAIL_LINE_MAX);
  strcpy (data + TAIL_LINE_MAX, "\n");
  log_append ("web", DP_LOG_STDOUT, data);
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  strcpy (expected, "web    | ");
  memset (expected + strlen (expected), 'x', TAIL_LINE_MAX);
  strcat (expected, "\nweb    | xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
                    "\nweb    | ");
  memset (expected + strlen (expected), 'y', TAIL_LINE_MAX);
  strcat (expected, "\n");
  expect (expected);

  // Pending data is printed when finishing.
  log_append ("worker", DP_LOG_STDERR, "last words");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("");
  TEST_ASSERT (dpcTailFlush (&tail) == 0);
  expect ("worker ! last words\n");

  dpcTailDestroy (&tail);
  TEST_ASSERT (tail == NULL);

  log_remove ("web", DP_LOG_STDOUT);
  log_remove ("web", DP_LOG_STDERR);
  log_remove ("worker", DP_LOG_STDOUT);
  log_remove ("worker", DP_LOG_STDERR);
  remove (rotated);
}

static void
test_backlog (const char *content, DPSize lines, const char *expected)
{
  log_remove ("solo", DP_LOG_STDOUT);
  log_append ("solo", DP_LOG_STDOUT, content);

  DPCTail tail = NULL;
  TEST_ASSERT (dpcTailInit (&tail, workdir, outfd) == 0);
  TEST_ASSERT (dpcTailAdd (&tail, "solo") == 0);
  TEST_ASSERT (dpcTailStart (&tail, lines) == 0);
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  TEST_ASSERT (dpcTailFlush (&tail) == 0);
  dpcTailDestroy (&tail);

  expect (expected);

  log_remove ("solo", DP_LOG_STDOUT);
}

static void
test_backlogs (void)
{
  test_backlog ("a\nb\nc\n", 0, "");
  test_backlog ("a\nb\nc\n", 1, "solo | c\n");
  test_backlog ("a\nb\nc\n", 100, "solo | a\nsolo | b\nsolo | c\n");
  test_backlog ("a\nb", 1, "solo | b\n");
  test_backlog ("a\n\n", 2, "solo | a\nsolo | \n");
  test_backlog ("", 10, "");

  // Backlog spanning multiple read chunks.
  static char content[OUT_MAX];
  static char expected[OUT_MAX];
  content[0] = '\0';
  expected[0] = '\0';
  for (int i = 0; i < 3000; ++i)
    {
      char line[64];
      snprintf (line, sizeof (line), "line %d\n", i);
      strcat (content, line);

      if (i >= 2000)
        {
          snprintf (line, sizeof (line), "solo | line %d\n", i);
          strcat (expected, line);
        }
    }
  test_backlog (content, 1000, expected);
}

static void
test_rotations (void)
{
  log_append ("chain", DP_LOG_STDOUT, "start\n");

  DPCTail tail = NULL;
  TEST_ASSERT (dpcTailInit (&tail, workdir, outfd) == 0);
  TEST_ASSERT (dpcTailAdd (&tail, "chain") == 0);
  TEST_ASSERT (dpcTailStart (&tail, 1) == 0);
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("chain | start\n");

  // Several rotations between two polls, the followed file is still among
  // the rotated ones.
  log_append ("chain", DP_LOG_STDOUT, "x1\n");
  log_rotate ("chain", 3);
  log_append ("chain", DP_LOG_STDOUT, "x2\n");
  log_rotate ("chain", 3);
  log_append ("chain", DP_LOG_STDOUT, "x3\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("chain | x1\n"
          "chain | x2\n"
          "chain | x3\n");

  // The followed file got rotated out meanwhile.
  log_append ("chain", DP_LOG_STDOUT, "y1\n");
  log_rotate ("chain", 2);
  log_append ("chain", DP_LOG_STDOUT, "y2\n");
  log_rotate ("chain", 2);
  log_append ("chain", DP_LOG_STDOUT, "y3\n");
  log_rotate ("chain", 2);
  log_append ("chain", DP_LOG_STDOUT, "y4\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("chain | y1\n"
          "chain | y2\n"
          "chain | y3\n"
          "chain | y4\n");

  // A line split by several rotations is joined.
  log_append ("chain", DP_LOG_STDOUT, "sp");
  log_rotate ("chain", 2);
  log_append ("chain", DP_LOG_STDOUT, "li");
  log_rotate ("chain", 2);
  log_append ("chain", DP_LOG_STDOUT, "t\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("chain | split\n");

  dpcTailDestroy (&tail);
  log_remove_all ("chain");

  // Rotated files left over from before are not printed, those created
  // after the start are.
  rot_write ("stale", 0, "old\n");

  TEST_ASSERT (dpcTailInit (&tail, workdir, outfd) == 0);
  TEST_ASSERT (dpcTailAdd (&tail, "stale") == 0);
  TEST_ASSERT (dpcTailAdd (&tail, "late") == 0);
  TEST_ASSERT (dpcTailStart (&tail, 10) == 0);
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("");

  log_append ("stale", DP_LOG_STDOUT, "new\n");
  log_append ("late", DP_LOG_STDOUT, "a\n");
  log_rotate ("late", 2);
  log_append ("late", DP_LOG_STDOUT, "b\n");
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  expect ("stale | new\n"
          "late  | a\n"
          "late  | b\n");

  dpcTailDestroy (&tail);
  log_remove_all ("stale");
  log_remove_all ("late");
}

// Files are given from the current one to the oldest rotated one.
static void
test_backlog_rot (const char *const files[], DPSize lines,
                  const char *expected)
{
  log_remove_all ("solo");
  log_append ("solo", DP_LOG_STDOUT, files[0]);
  for (int i = 1; files[i] != NULL; ++i)
    {
      rot_write ("solo", i - 1, files[i]);
    }

  DPCTail tail = NULL;
  TEST_ASSERT (dpcTailInit (&tail, workdir, outfd) == 0);
  TEST_ASSERT (dpcTailAdd (&tail, "solo") == 0);
  TEST_ASSERT (dpcTailStart (&tail, lines) == 0);
  TEST_ASSERT (dpcTailPoll (&tail) == 0);
  TEST_ASSERT (dpcTailFlush (&tail) == 0);
  dpcTailDestroy (&tail);

  expect (expected);

  log_remove_all ("solo");
}

static void
test_backlogs_rot (void)
{
  const char *const files[] = { "e\nf\n", "c\nd", "a\nb\n", NULL };
  test_backlog_rot (files, 1, "solo | f\n");
  test_backlog_rot (files, 4, "solo | b\nsolo | c\nsolo | de\nsolo | f\n");
  test_backlog_rot (files, 100, "solo | a\nsolo | b\nsolo | c\nsolo | de\n"
                                "solo | f\n");

  // The newline terminating the log may be the last one of a rotated file.
  const char *const empty[] = { "", "x\ny\n", NULL };
  test_backlog_rot (empty, 1, "solo | y\n");
  test_backlog_rot (empty, 2, "solo | x\nsolo | y\n");
}

static void
test_errors (void)
{
  DPCTail tail = NULL;
  TEST_ASSERT (dpcTailInit (&tail, workdir, outfd) == 0);

  // Nothing to follow.
  TEST_ASSERT (dpcTailStart (&tail, 10) < 0);

  dpcTailDestroy (&tail);
  dpcTailDestroy (&tail);
}

int
main (int argc, const char *argv[])
{
  (void)(argc);
  (void)(argv);

  TEST_ASSERT (mkdtemp (workdir) != NULL);

  char logdir[64];
  snprintf (logdir, sizeof (logdir), "%s/log", workdir);
  TEST_ASSERT (mkdir (logdir, S_IRWXU) == 0);

  char outpath[64];
  snprintf (outpath, sizeof (outpath), "%s/out.txt", workdir);
  outfd = open (outpath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  TEST_ASSERT (outfd >= 0);

  test_follow ();
  test_backlogs ();
  test_rotations ();
  test_backlogs_rot ();
  test_errors ();

  close (outfd);
  remove (outpath);
  rmdir (logdir);
  rmdir (workdir);

  return 0;
}
