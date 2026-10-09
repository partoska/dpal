/*
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

#ifndef _DPALTYPES_H_
#define _DPALTYPES_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define DP_API extern
#define DP_VERSION "1.3.2"

#define DP_NAME_MAX (64)
#define DP_APP_MAX (1024)
#define DP_DIR_MAX (1024)
#define DP_ARG_MAX (1024)
#define DP_ENV_MAX (1024)

#ifdef __cplusplus
extern "C"
{
#endif

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * Types
   * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

  typedef char DPChar;

  typedef signed char DPSByte;
  typedef signed short DPShort;
  typedef signed int DPInt;
  typedef signed long DPLong;

  typedef unsigned char DPByte;
  typedef unsigned short DPWord;
  typedef unsigned int DPDword;
  typedef unsigned long DPQword;

  typedef DPByte DPBool;

  typedef DPInt DPId;
  typedef struct DPSProcDef *DPProcDef;
  typedef struct DPSProcAttr *DPProcAttr;
  typedef struct DPSProcInfo *DPProcInfo;
  typedef DPId DPProcIter;

  typedef enum DPEStatus
  {
    DP_OK = 0x00,
    DP_BREAK = 0x01,

    DP_EINIT = 0x10,
    DP_EALLOC,
    DP_ENULL,
    DP_EINVALID,
    DP_ENOT_FOUND,
    DP_EACCESS,
    DP_EDOUBLE_INIT,
    DP_EPIPE,
    DP_ECLOSE,
    DP_EREAD,
    DP_EWRITE,
    DP_EWAIT,
    DP_EPID,
    DP_EFD,
    DP_EPROCESS_CREATE,
    DP_EPROCESS_STOP,
    DP_ELOG,
    DP_ETOOLONG,
    DP_ENOT_STOPPED,
    DP_EBOUND,
    DP_EEXIST,
    DP_ESELECT,
    DP_ECONF,

    DP_ENOT_IMPLEMENTED = 0xFE,

    DP_EUNEXPECTED = 0xFF,
  } DPStatus;

  typedef enum DPEProcState
  {
    DP_STOPPED = 0x00,
    DP_RUNNING = 0x10
  } DPProcState;

  typedef DPInt DPPid;
  typedef DPInt DPFd;
  typedef DPQword DPSize;
  typedef DPLong DPSSize;

#ifdef __cplusplus
}
#endif

#endif
