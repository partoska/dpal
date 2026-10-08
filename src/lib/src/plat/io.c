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

#include <fcntl.h>
#include <unistd.h>

#include "plat/io.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

const DPFd DP_FD_STDOUT = STDOUT_FILENO;
const DPFd DP_FD_STDERR = STDERR_FILENO;
const DPFd DP_FD_STDIN = STDIN_FILENO;
const DPFd DP_FD_INVALID = -1;

const DPDword DP_O_CREAT = O_CREAT;
const DPDword DP_O_APPEND = O_APPEND;
const DPDword DP_O_RDWR = O_RDWR;
const DPDword DP_O_RDONLY = O_RDONLY;
const DPDword DP_O_WRONLY = O_WRONLY;
const DPDword DP_O_TRUNC = O_TRUNC;
const DPDword DP_S_IRUSR = S_IRUSR;
const DPDword DP_S_IWUSR = S_IWUSR;
const DPDword DP_S_IRWXU = S_IRWXU;

DPBool
dpIsFdValid (DPFd fd)
{
  return fd != DP_FD_INVALID;
}

DPFd
dpOpen (const DPChar *pathname, DPInt flags, DPMode mode)
{
  return open (pathname, flags, mode);
}

DPSSize
dpRead (DPFd fd, void *dst, DPSize n)
{
  return read (fd, dst, n);
}

DPSSize
dpWrite (DPFd fd, const void *src, DPSize n)
{
  return write (fd, src, n);
}

DPInt
dpClose (DPFd fd)
{
  return close (fd);
}

DPFd
dpPipe (DPFd pipefd[2])
{
  return pipe (pipefd);
}

DPFd
dpDup2 (DPFd oldfd, DPFd newfd)
{
  return dup2 (oldfd, newfd);
}
