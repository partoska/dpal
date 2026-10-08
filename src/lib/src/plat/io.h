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

#ifndef _PLAT_IO_H_
#define _PLAT_IO_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dpaltypes.h"

#include "plat/types.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API const DPFd DP_FD_STDOUT;
DP_API const DPFd DP_FD_STDERR;
DP_API const DPFd DP_FD_STDIN;
DP_API const DPFd DP_FD_INVALID;

DP_API const DPDword DP_O_CREAT;
DP_API const DPDword DP_O_APPEND;
DP_API const DPDword DP_O_RDWR;
DP_API const DPDword DP_O_RDONLY;
DP_API const DPDword DP_O_WRONLY;
DP_API const DPDword DP_O_TRUNC;
DP_API const DPDword DP_S_IRUSR;
DP_API const DPDword DP_S_IWUSR;
DP_API const DPDword DP_S_IRWXU;

DP_API DPBool dpIsFdValid (DPFd fd);

DP_API DPFd dpOpen (const DPChar *pathname, DPInt flags, DPMode mode);
DP_API DPSSize dpRead (DPFd fd, void *dst, DPSize n);
DP_API DPSSize dpWrite (DPFd fd, const void *src, DPSize n);
DP_API DPInt dpClose (DPFd fd);

DP_API DPFd dpPipe (DPFd[2]);
DP_API DPFd dpDup2 (DPFd oldfd, DPFd newfd);

#endif
