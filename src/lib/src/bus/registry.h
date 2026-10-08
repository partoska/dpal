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

#ifndef _BUS_REGISTRY_H_
#define _BUS_REGISTRY_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "core/info.h"
#include "core/log.h"
#include "plat/common.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef DPStatus DPOnVisitErrorCb (DPStatus);
typedef DPStatus DPRegVisitor (DPId id);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * @brief Registers a new process into the process manager.
 *
 * @param[in,out] id Process identifier assigned during registration. It is
 * possible to provide a positive number (including zero) as a suggestion. If
 * such an identifier is already allocated, the next free identifier higher
 * than the provided one is utilized.
 *
 * @param[in] def Process definition. An internal clone of definition is
 * created during registration. Therefore, the provided structure still must
 * be cleaned up using dpProcDefDestroy() once not needed.
 *
 * @param[in] attr Process attributes. An internal clone of attributes is
 * created during registration. Therefore, the provided structure still must
 * be cleaned up using dpProcAttrDestroy() once not needed.
 *
 * @return On success, the function return 0; on error, it returns a negative
 * error number.
 */
DP_API DPStatus dpProcReg (DPId *id, const DPProcDef *def,
                           const DPProcAttr *attr);
DP_API DPStatus dpProcUnreg (DPId id);
DP_API DPStatus dpProcGetDef (DPProcDef *def, DPId id);
DP_API DPStatus dpProcGetAttr (DPProcAttr *attr, DPId id);
DP_API DPStatus dpProcGetInfo (DPProcInfo *info, DPId id);
DP_API DPStatus dpProcExists (DPId id);

/**
 * @brief Finds the process identifier associated with the given name.
 *
 * This function searches for the specified name in the process registry and,
 * if found, returns the associated identifier. If the name is found, the
 * function returns DP_OK. Otherwise, if the name is not found or is invalid,
 * the function returns DP_BREAK. Moreover, if the found parameter is not
 * NULL and the name is found, the associated identifier is stored in the
 * location pointed to by found.
 *
 * @param[out] found Pointer to a DPId where the found process identifier
 * will be stored, if not NULL.
 *
 * @param[in] name The name to search for in the process registry.
 *
 * @return DPStatus DP_OK if the name is found. DP_BREAK if the name is not
 * found or NULL.
 *
 * @note The function will also return DP_BREAK if the system is not properly
 * initialized.
 */
DP_API DPStatus dpProcFind (DPId *found, const DPChar *name);
DP_API DPStatus dpProcIter (DPProcIter *iter);
DP_API DPStatus dpProcIterNext (DPProcIter *iter, DPId *next);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Internal
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

DP_API DPStatus dpRegInit (void);
DP_API DPStatus dpRegDestroy (void);
DP_API DPStatus dpRegEach (DPRegVisitor visitor, DPOnVisitErrorCb cb);
DP_API DPStatus dpProcGetProps (DPId id, DPProcDef *def, DPProcAttr *attr,
                                DPProcInfo *info);
DP_API DPStatus dpProcRefProps (DPId id, const DPProcDef **def,
                                const DPProcAttr **attr,
                                const DPProcInfoEx **info);
DP_API DPStatus dpProcPtrInfoEx (DPId id, DPProcInfoEx **info);
DP_API DPStatus dpProcRefInfoEx (DPId id, const DPProcInfoEx **info);
DP_API DPStatus dpProcPtrLog (DPId id, DPLog **out, DPLog **err);
DP_API DPStatus dpProcRefLog (DPId id, const DPLog **out, const DPLog **err);

DP_API DPStatus dpVisitContinueCb (DPStatus error);
DP_API DPStatus dpVisitBreakCb (DPStatus error);

#endif
