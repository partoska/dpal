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

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "plat/filesys.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DPInt
dpChdir (const DPChar *path)
{
  return chdir (path);
}

DPInt
dpMkdir (const DPChar *path, DPMode mode)
{
  return mkdir (path, mode);
}

DPInt
dpUnlink (const DPChar *path)
{
  return unlink (path);
}

DPInt
dpRename (const DPChar *old, const DPChar *new)
{
  return rename (old, new);
}

DPBool
dpDirExists (const DPChar *dir)
{
  struct stat sb;
  return stat (dir, &sb) == 0 && S_ISDIR (sb.st_mode);
}

DPBool
dpFileExists (const DPChar *file)
{
  struct stat sb;
  return stat (file, &sb) == 0 && S_ISREG (sb.st_mode);
}

DPQword
dpFileSize (const DPChar *file)
{
  struct stat sb;
  if (stat (file, &sb) != 0)
    {
      return 0;
    }

  if (sb.st_size < 0)
    {
      return 0;
    }

  return (DPQword)sb.st_size;
}
