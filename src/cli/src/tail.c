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

#include "dpal.h"

#include "check.h"
#include "logger.h"
#include "plat.h"
#include "tail.h"
#include "util.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define TAIL_READ_MAX (4096)
#define TAIL_DRAIN_MAX (256)
#define TAIL_OUT_MAX (16384)
#define TAIL_PATH_MAX ((WORKDIR_MAX) + (DP_LOG_PATH_MAX))
#define TAIL_ROT_PATH_MAX ((TAIL_PATH_MAX) + 8)
#define TAIL_ROT_MAX (256)
#define TAIL_MARKER_LEN (3)
#define TAIL_IDLE_POLLS (1)
#define TAIL_FD_INVALID (-1)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Asserts
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

COMPILE_ASSERT (DP_NAME_MAX + TAIL_MARKER_LEN + TAIL_LINE_MAX + 1
                <= TAIL_OUT_MAX);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct DPCTailStream
{
  DPChar name[DP_NAME_MAX];
  DPChar path[TAIL_PATH_MAX];
  DPLogType type;
  DPFd fd;
  DPCFileStat last;
  DPCFileStat head;
  DPQword off;
  DPChar line[TAIL_LINE_MAX];
  DPSize len;
  DPByte idle;
  DPBool cut;
} DPCTailStream;

struct DPCSTail
{
  DPChar workdir[WORKDIR_MAX];
  DPFd out;
  DPCTailStream *streams;
  DPSize count;
  DPSize width;
  DPChar buff[TAIL_OUT_MAX];
  DPSize used;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static const DPChar STDOUT_MARKER[] = " | ";
static const DPChar STDERR_MARKER[] = " ! ";

COMPILE_ASSERT (CHARSMAX (STDOUT_MARKER) == TAIL_MARKER_LEN);
COMPILE_ASSERT (CHARSMAX (STDERR_MARKER) == TAIL_MARKER_LEN);

// ===============================================================
// Output
// ===============================================================

static DPInt
dpcTailWrite (DPCTail *tail)
{
  const DPChar *ptr = (*tail)->buff;
  DPSize remain = (*tail)->used;
  (*tail)->used = 0;

  while (remain > 0)
    {
      DPSSize written = dpcWrite ((*tail)->out, ptr, remain);
      CHECK_POSIX (written, "Cannot write log output", RETURN_WITH_FAILURE);

      ptr += written;
      remain -= written;
    }

  return DPC_OK;
}

static DPInt
dpcTailEmit (DPCTail *tail, DPCTailStream *stream)
{
  const DPSize nameLen = dpcStrlen (stream->name);
  const DPSize size = (*tail)->width + TAIL_MARKER_LEN + stream->len + 1;
  if ((*tail)->used + size > SIZEOF ((*tail)->buff))
    {
      if (dpcTailWrite (tail) < 0)
        {
          return DPC_EFAIL;
        }
    }

  // Prefix the line with the name padded to the longest followed name and
  // a marker of the captured stream.
  DPChar *ptr = (*tail)->buff + (*tail)->used;
  dpcMemcpy (ptr, stream->name, nameLen);
  ptr += nameLen;
  dpcMemset (ptr, ' ', (*tail)->width - nameLen);
  ptr += (*tail)->width - nameLen;
  dpcMemcpy (ptr,
             stream->type == DP_LOG_STDOUT ? STDOUT_MARKER : STDERR_MARKER,
             TAIL_MARKER_LEN);
  ptr += TAIL_MARKER_LEN;
  dpcMemcpy (ptr, stream->line, stream->len);
  ptr += stream->len;
  *ptr = '\n';

  (*tail)->used += size;
  stream->len = 0;
  stream->idle = 0;

  return DPC_OK;
}

static DPInt
dpcTailCut (DPCTail *tail, DPCTailStream *stream)
{
  if (dpcTailEmit (tail, stream) < 0)
    {
      return DPC_EFAIL;
    }

  // The line continues, so its newline must not produce an empty line.
  stream->cut = TRUE;

  return DPC_OK;
}

static DPInt
dpcTailFeed (DPCTail *tail, DPCTailStream *stream, const DPChar *data,
             DPSize size)
{
  for (DPSize i = 0; i < size; ++i)
    {
      if (data[i] == '\n')
        {
          if (stream->cut)
            {
              stream->cut = FALSE;
              continue;
            }

          if (dpcTailEmit (tail, stream) < 0)
            {
              return DPC_EFAIL;
            }

          continue;
        }

      stream->cut = FALSE;
      stream->line[stream->len++] = data[i];

      if (stream->len == SIZEOF (stream->line))
        {
          if (dpcTailCut (tail, stream) < 0)
            {
              return DPC_EFAIL;
            }
        }
    }

  return DPC_OK;
}

// ===============================================================
// Log files
// ===============================================================

static void
dpcTailRotPath (const DPCTailStream *stream, DPInt idx, DPChar *path)
{
  // Rotated log files carry a numeric suffix, the higher the older.
  dpcMemset (path, 0x0, TAIL_ROT_PATH_MAX);
  dpcStrncpy (path, stream->path, CHARSMAX (stream->path));

  DPSize len = dpcStrlen (path);
  path[len] = '.';
  dpcItoa10 (idx, path + len + 1);
}

static DPFd
dpcTailOpenFile (const DPChar *path, DPCFileStat *st)
{
  DPFd fd = dpcOpenRead (path);
  if (fd < 0)
    {
      return TAIL_FD_INVALID;
    }

  if (dpcFstat (fd, st) < 0)
    {
      dpcClose (fd);

      return TAIL_FD_INVALID;
    }

  return fd;
}

static DPInt
dpcTailOpen (DPCTailStream *stream)
{
  DPCFileStat st;
  DPFd fd = dpcTailOpenFile (stream->path, &st);
  if (fd == TAIL_FD_INVALID)
    {
      return DPC_EFAIL;
    }

  stream->fd = fd;
  stream->last = st;
  stream->off = 0;

  return DPC_OK;
}

static void
dpcTailClose (DPCTailStream *stream)
{
  if (stream->fd != TAIL_FD_INVALID)
    {
      dpcClose (stream->fd);
      stream->fd = TAIL_FD_INVALID;
    }
}

static DPBool
dpcTailSameFile (const DPCFileStat *a, const DPCFileStat *b)
{
  return a->ino != 0 && a->dev == b->dev && a->ino == b->ino;
}

static void
dpcTailHead (DPCTailStream *stream)
{
  // Remember the newest rotated file, files rotated after it are new.
  DPChar path[TAIL_ROT_PATH_MAX];
  dpcTailRotPath (stream, 0, path);
  if (dpcStat (path, &stream->head) < 0)
    {
      dpcMemset (&stream->head, 0x0, SIZEOF (stream->head));
    }
}

static DPInt
dpcTailRead (DPCTail *tail, DPCTailStream *stream, DPFd fd, DPQword *bytes,
             DPBool *eof)
{
  DPChar data[TAIL_READ_MAX];

  // Bound the amount of data read at once, so a busy log file does not
  // starve the others.
  *eof = FALSE;
  for (DPDword i = 0; i < TAIL_DRAIN_MAX; ++i)
    {
      DPSSize n = dpcRead (fd, data, SIZEOF (data));
      CHECK_POSIX (n, "Cannot read log file", RETURN_WITH_FAILURE);

      if (n == 0)
        {
          *eof = TRUE;
          break;
        }

      *bytes += n;

      if (dpcTailFeed (tail, stream, data, n) < 0)
        {
          return DPC_EFAIL;
        }
    }

  return DPC_OK;
}

static DPInt
dpcTailReadAll (DPCTail *tail, DPCTailStream *stream, DPFd fd,
                DPQword *bytes)
{
  DPBool eof = FALSE;
  while (!eof)
    {
      if (dpcTailRead (tail, stream, fd, bytes, &eof) < 0)
        {
          return DPC_EFAIL;
        }
    }

  return DPC_OK;
}

static DPInt
dpcTailDrain (DPCTail *tail, DPCTailStream *stream, DPBool *got, DPBool *eof)
{
  DPQword bytes = 0;
  if (dpcTailRead (tail, stream, stream->fd, &bytes, eof) < 0)
    {
      return DPC_EFAIL;
    }

  stream->off += bytes;
  if (bytes > 0)
    {
      *got = TRUE;
    }

  return DPC_OK;
}

static DPInt
dpcTailReadFull (DPFd fd, DPChar *data, DPSize size)
{
  while (size > 0)
    {
      DPSSize bytes = dpcRead (fd, data, size);
      if (bytes <= 0)
        {
          return DPC_EFAIL;
        }

      data += bytes;
      size -= bytes;
    }

  return DPC_OK;
}

static DPBool
dpcTailSeekLines (DPFd fd, DPQword size, DPSize lines, DPSize *found,
                  DPBool *atEnd, DPQword *off)
{
  // Scan the file backwards for the start of the requested lines. The count
  // continues the one of the newer file scanned before.
  DPChar data[TAIL_READ_MAX];
  DPQword end = size;
  while (end > 0)
    {
      const DPQword start = end > SIZEOF (data) ? end - SIZEOF (data) : 0;
      const DPSize n = end - start;
      if (dpcSeek (fd, start) < 0 || dpcTailReadFull (fd, data, n) < 0)
        {
          // Do not print anything old rather than everything.
          *off = size;

          return TRUE;
        }

      for (DPSize i = n; i > 0; --i)
        {
          const DPChar c = data[i - 1];

          // A newline terminating the log does not start another line.
          if (*atEnd)
            {
              *atEnd = FALSE;
              if (c == '\n')
                {
                  continue;
                }
            }

          if (c != '\n')
            {
              continue;
            }

          if (++*found == lines)
            {
              *off = start + i;

              return TRUE;
            }
        }

      end = start;
    }

  *off = 0;

  return FALSE;
}

static DPInt
dpcTailBacklog (DPCTail *tail, DPCTailStream *stream, DPSize lines)
{
  DPSize found = 0;
  DPBool atEnd = TRUE;
  DPQword off = stream->last.size;
  if (lines == 0
      || dpcTailSeekLines (stream->fd, stream->last.size, lines, &found,
                           &atEnd, &off))
    {
      CHECK_POSIX (dpcSeek (stream->fd, off), "Cannot seek log file",
                   RETURN_WITH_FAILURE);
      stream->off = off;

      return DPC_OK;
    }

  // The current file holds fewer lines, continue in the rotated ones. They
  // are kept open, so a rotation cannot shift them meanwhile.
  DPFd fds[TAIL_ROT_MAX];
  DPInt count = 0;
  DPBool complete = FALSE;
  DPQword start = 0;
  while (count < TAIL_ROT_MAX && !complete)
    {
      DPChar path[TAIL_ROT_PATH_MAX];
      dpcTailRotPath (stream, count, path);

      DPCFileStat st;
      DPFd fd = dpcTailOpenFile (path, &st);
      if (fd == TAIL_FD_INVALID)
        {
          break;
        }

      // The current file got rotated meanwhile.
      if (dpcTailSameFile (&stream->last, &st))
        {
          dpcClose (fd);
          break;
        }

      fds[count++] = fd;
      complete
          = dpcTailSeekLines (fd, st.size, lines, &found, &atEnd, &start);
    }

  // Print the oldest file from the first requested line, the newer ones
  // whole, and the current one is followed from its beginning.
  DPInt result = DPC_OK;
  for (DPInt i = count - 1; i >= 0; --i)
    {
      DPQword bytes = 0;
      const DPQword from = (i == count - 1 && complete) ? start : 0;
      if (result == DPC_OK)
        {
          result = dpcSeek (fds[i], from) < 0
                       ? DPC_EFAIL
                       : dpcTailReadAll (tail, stream, fds[i], &bytes);
        }

      dpcClose (fds[i]);
    }

  CHECK_POSIX (dpcSeek (stream->fd, 0), "Cannot seek log file",
               RETURN_WITH_FAILURE);
  stream->off = 0;

  return result;
}

static DPInt
dpcTailCatchUp (DPCTail *tail, DPCTailStream *stream, DPBool *got)
{
  // Several rotations can happen between two polls. The rotated files newer
  // than the last read one are those before it. When it is not rotated
  // anymore (removed or rotated out), those before the formerly newest
  // rotated file are. When that one is gone too, all of them are new.
  DPChar path[TAIL_ROT_PATH_MAX];
  DPCFileStat st;
  DPInt lastIdx = -1;
  DPInt headIdx = -1;
  DPInt count = 0;
  while (count < TAIL_ROT_MAX)
    {
      dpcTailRotPath (stream, count, path);
      if (dpcStat (path, &st) < 0)
        {
          break;
        }

      if (lastIdx < 0 && dpcTailSameFile (&stream->last, &st))
        {
          lastIdx = count;
        }

      if (headIdx < 0 && dpcTailSameFile (&stream->head, &st))
        {
          headIdx = count;
        }

      ++count;
    }

  const DPInt newer = lastIdx >= 0 ? lastIdx : headIdx >= 0 ? headIdx : count;

  // Read them from the oldest one.
  for (DPInt i = newer - 1; i >= 0; --i)
    {
      dpcTailRotPath (stream, i, path);
      DPFd fd = dpcTailOpenFile (path, &st);
      if (fd == TAIL_FD_INVALID)
        {
          continue;
        }

      DPQword bytes = 0;
      DPInt result = dpcTailReadAll (tail, stream, fd, &bytes);
      dpcClose (fd);
      if (result < 0)
        {
          return DPC_EFAIL;
        }

      if (bytes > 0)
        {
          *got = TRUE;
        }
    }

  if (count > 0)
    {
      dpcTailHead (stream);
    }

  return DPC_OK;
}

static DPInt
dpcTailPollStream (DPCTail *tail, DPCTailStream *stream)
{
  DPBool got = FALSE;
  DPBool eof = FALSE;

  // Check the path before draining. When a rotation is observed here, the
  // old file is already complete, so nothing gets lost by switching to the
  // newer ones once the old one is drained.
  DPCFileStat cur;
  const DPBool exists = dpcStat (stream->path, &cur) == 0;

  if (stream->fd == TAIL_FD_INVALID)
    {
      // Catch up with the rotations since the last read file, then follow
      // the current one from its beginning.
      if (exists)
        {
          if (dpcTailCatchUp (tail, stream, &got) < 0)
            {
              return DPC_EFAIL;
            }

          if (dpcTailOpen (stream) == DPC_OK
              && dpcTailDrain (tail, stream, &got, &eof) < 0)
            {
              return DPC_EFAIL;
            }
        }
    }
  else
    {
      // The file was truncated in place, start over.
      DPCFileStat st;
      if (dpcFstat (stream->fd, &st) == 0 && st.size < stream->off)
        {
          CHECK_POSIX (dpcSeek (stream->fd, 0), "Cannot seek log file",
                       RETURN_WITH_FAILURE);
          stream->off = 0;
        }

      if (dpcTailDrain (tail, stream, &got, &eof) < 0)
        {
          return DPC_EFAIL;
        }

      // The file was rotated or removed. A line split by the rotation
      // continues in the newer files, so its pending part is kept.
      if (eof && (!exists || !dpcTailSameFile (&stream->last, &cur)))
        {
          dpcTailClose (stream);

          if (exists)
            {
              if (dpcTailCatchUp (tail, stream, &got) < 0)
                {
                  return DPC_EFAIL;
                }

              if (dpcTailOpen (stream) == DPC_OK
                  && dpcTailDrain (tail, stream, &got, &eof) < 0)
                {
                  return DPC_EFAIL;
                }
            }
        }
    }

  if (got)
    {
      stream->idle = 0;

      return DPC_OK;
    }

  // Print an incomplete line (e.g. a prompt) once no more data arrives.
  if (stream->len > 0 && ++stream->idle >= TAIL_IDLE_POLLS)
    {
      return dpcTailCut (tail, stream);
    }

  return DPC_OK;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPInt
dpcTailInit (DPCTail *tail, const DPChar *workdir, DPFd out)
{
  CHECK_GENERIC (dpcStrnlen (workdir, WORKDIR_MAX) < WORKDIR_MAX,
                 "Work directory path is too long", RETURN_WITH_FAILURE);

  *tail = (DPCTail)dpcCalloc (1, SIZEOF (struct DPCSTail));
  CHECK_GENERIC (*tail != NULL, "Cannot allocate log follower",
                 RETURN_WITH_FAILURE);

  dpcStrncpy ((*tail)->workdir, workdir, CHARSMAX ((*tail)->workdir));
  (*tail)->out = out;

  return DPC_OK;
}

void
dpcTailDestroy (DPCTail *tail)
{
  if (*tail == NULL)
    {
      return;
    }

  for (DPSize i = 0; i < (*tail)->count; ++i)
    {
      dpcTailClose (&(*tail)->streams[i]);
    }

  dpcFree ((*tail)->streams);
  dpcFree (*tail);
  *tail = NULL;
}

DPInt
dpcTailAdd (DPCTail *tail, const DPChar *name)
{
  // Each process is followed only once.
  for (DPSize i = 0; i < (*tail)->count; ++i)
    {
      if (dpcStrcmp ((*tail)->streams[i].name, name) == 0)
        {
          return DPC_OK;
        }
    }

  const DPLogType types[] = { DP_LOG_STDOUT, DP_LOG_STDERR };

  DPCTailStream *streams = (DPCTailStream *)dpcRealloc (
      (*tail)->streams,
      ((*tail)->count + COUNTOF (types)) * SIZEOF (DPCTailStream));
  CHECK_GENERIC (streams != NULL, "Cannot allocate log follower",
                 RETURN_WITH_FAILURE);
  (*tail)->streams = streams;

  for (DPSize i = 0; i < COUNTOF (types); ++i)
    {
      DPCTailStream *stream = &streams[(*tail)->count + i];
      dpcMemset (stream, 0x0, SIZEOF (*stream));
      stream->type = types[i];
      stream->fd = TAIL_FD_INVALID;

      // Log files live in the work directory of the control process.
      dpcStrncpy (stream->path, (*tail)->workdir, CHARSMAX (stream->path));
      DPSize len = dpcStrlen (stream->path);
      if (len > 0 && stream->path[len - 1] != '/')
        {
          stream->path[len++] = '/';
        }

      CHECK_DPAL (dpLogPath (name, stream->type, stream->path + len,
                             SIZEOF (stream->path) - len, NULL),
                  "Invalid process name", RETURN_WITH_FAILURE);

      dpcStrncpy (stream->name, name, CHARSMAX (stream->name));
    }

  (*tail)->count += COUNTOF (types);
  (*tail)->width = MAX ((*tail)->width, dpcStrlen (name));

  return DPC_OK;
}

DPInt
dpcTailStart (DPCTail *tail, DPSize lines)
{
  CHECK_GENERIC ((*tail)->count > 0, "No processes to follow",
                 RETURN_WITH_FAILURE);

  for (DPSize i = 0; i < (*tail)->count; ++i)
    {
      DPCTailStream *stream = &(*tail)->streams[i];

      // Missing log files are picked up by the poll once they appear,
      // together with the files they get rotated to from now on.
      dpcTailHead (stream);
      if (dpcTailOpen (stream) < 0)
        {
          continue;
        }

      if (dpcTailBacklog (tail, stream, lines) < 0)
        {
          return DPC_EFAIL;
        }
    }

  return dpcTailWrite (tail);
}

DPInt
dpcTailPoll (DPCTail *tail)
{
  for (DPSize i = 0; i < (*tail)->count; ++i)
    {
      if (dpcTailPollStream (tail, &(*tail)->streams[i]) < 0)
        {
          return DPC_EFAIL;
        }
    }

  return dpcTailWrite (tail);
}

DPInt
dpcTailFlush (DPCTail *tail)
{
  for (DPSize i = 0; i < (*tail)->count; ++i)
    {
      DPCTailStream *stream = &(*tail)->streams[i];
      if (stream->len > 0 && dpcTailCut (tail, stream) < 0)
        {
          return DPC_EFAIL;
        }
    }

  return dpcTailWrite (tail);
}
