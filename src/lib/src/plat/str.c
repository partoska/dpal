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

#include <libgen.h>
#include <stdlib.h>
#include <string.h>

#include "dpaltypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void *
dpMemset (void *dst, DPInt val, DPSize n)
{
  return memset (dst, val, n);
}

void *
dpMemcpy (void *restrict dst, const void *restrict src, DPSize n)
{
  return memcpy (dst, src, n);
}

DPSize
dpStrlen (const DPChar *src)
{
  return strlen (src);
}

DPSize
dpStrnlen (const DPChar *src, DPSize max)
{
  return strnlen (src, max);
}

DPInt
dpStrcmp (const DPChar *src1, const DPChar *src2)
{
  return strcmp (src1, src2);
}

DPInt
dpStrncmp (const DPChar *src1, const DPChar *src2, DPSize n)
{
  return strncmp (src1, src2, n);
}

DPChar *
dpStrncpy (DPChar *dst, const DPChar *src, DPSize n)
{
  return strncpy (dst, src, n);
}

DPChar *
dpStrndup (const DPChar *src, DPSize n)
{
  return strndup (src, n);
}

DPChar *
dpStrncat (DPChar *dst, const DPChar *src, DPSize n)
{
  return strncat (dst, src, n);
}

DPQword
dpStrtoul (const DPChar *restrict nptr, DPChar **restrict endptr, DPInt base)
{
  return strtoul (nptr, endptr, base);
}

DPLong
dpStrtol (const DPChar *restrict nptr, DPChar **restrict endptr, DPInt base)
{
  return strtol (nptr, endptr, base);
}

static void
dpReverse (DPChar *str)
{
  DPInt len = dpStrlen (str);
  for (DPInt i = 0, j = len - 1; i < j; ++i, --j)
    {
      DPChar aux = str[i];
      str[i] = str[j];
      str[j] = aux;
    }
}

DPChar *
dpItoa10 (DPInt num, DPChar *str)
{
  DPInt i = 0;
  if (num < 0)
    {
      str[i] = '\0';
      return str;
    }

  if (num == 0)
    {
      str[i] = '0';
      ++i;
      str[i] = '\0';
      return str;
    }

  while (num != 0)
    {
      DPInt rem = num % 10;
      str[i] = rem + '0';
      ++i;
      num = num / 10;
    }

  str[i] = '\0';

  dpReverse (str);

  return str;
}

DPChar *
dpBasename (DPChar *path)
{
  return basename (path);
}
