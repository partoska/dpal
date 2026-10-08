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

#include <sys/select.h>
#include <unistd.h>

#include "dpaltypes.h"

#include "plat/alloc.h"
#include "plat/common.h"
#if _DEBUG
#include "plat/io.h"
#endif
#include "plat/logger.h"
#include "plat/select.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define ONE_SECOND (1000 * 1000)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct DPFdSet
{
  fd_set set;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPInt
dpSelect (DPInt nfds, DPFdSet *readfds, DPFdSet *writefds, DPFdSet *exceptfds,
          const DPUSeconds *timeout)
{
  struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };

  if (timeout != NULL)
    {
      tv.tv_sec = *timeout / ONE_SECOND;
      tv.tv_usec = *timeout % ONE_SECOND;
    }

  return select (nfds, readfds == NULL ? NULL : &(*readfds)->set,
                 writefds == NULL ? NULL : &(*writefds)->set,
                 exceptfds == NULL ? NULL : &(*exceptfds)->set,
                 timeout == NULL ? NULL : &tv);
}

DPInt
dpFdsInit (DPFdSet *fds)
{
  if (fds == NULL)
    {
      return -1;
    }

  *fds = (DPFdSet)dpMalloc (SIZEOF (struct DPFdSet));
  if (*fds == NULL)
    {
      return -1;
    }

  return 0;
}

DPInt
dpFdsDestroy (DPFdSet *fds)
{
  if (fds == NULL || *fds == NULL)
    {
      return -1;
    }

  dpFree (*fds);

  return 0;
}

void
dpFdsZero (DPFdSet *fds)
{
  if (fds == NULL || *fds == NULL)
    {
      DP_ERROR ("Zeroized file descriptor set is NULL!");

      return;
    }

  FD_ZERO (&(*fds)->set);
}

void
dpFdsSet (DPFd fd, DPFdSet *fds)
{
#if _DEBUG
  if (!dpIsFdValid (fd))
    {
      DP_ERROR ("File descriptor being set is invalid!");
    }
#endif

  if (fds == NULL || *fds == NULL)
    {
      DP_ERROR ("Modified file descriptor set is NULL!");

      return;
    }

  FD_SET (fd, &(*fds)->set);
}

DPInt
dpFdsIsSet (DPFd fd, DPFdSet *fds)
{
#if _DEBUG
  if (!dpIsFdValid (fd))
    {
      DP_ERROR ("File descriptor being checked is invalid!");
    }
#endif

  if (fds == NULL || *fds == NULL)
    {
      DP_ERROR ("Checked file descriptor set is NULL!");

      return FALSE;
    }

  return FD_ISSET (fd, &(*fds)->set);
}
