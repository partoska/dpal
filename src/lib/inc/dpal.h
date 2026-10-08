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

#ifndef _DPAL_H_
#define _DPAL_H_

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "dpaltypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef __cplusplus
extern "C"
{
#endif

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * Declarations
   * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

  /**
   * @brief Initializes the process manager library.
   *
   * This function must be called before any other library functions. It
   * initializes internal data structures, allocates resources, and prepares
   * the process manager for operation.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number. Returns DP_EDOUBLE_INIT if already initialized.
   */
  DP_API DPStatus dpInit (void);
  /**
   * @brief Processes one tick of the process manager.
   *
   * This function handles the core process management lifecycle including:
   * starting scheduled processes, stopping processes marked for termination,
   * reading output from running processes, waiting for process exits,
   * and scheduling restarts for failed processes.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpTick (void);
  /**
   * @brief Cleans up and destroys the process manager.
   *
   * This function deallocates all resources, closes file descriptors, and
   * cleans up internal data structures. All registered processes must be
   * stopped before calling this function.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpDestroy (void);
  /**
   * @brief Sets a callback function to be executed before process execution.
   *
   * The callback is invoked in the child process right before execvp() is
   * called. This allows for last-minute setup or cleanup operations.
   *
   * @param[in] handler Function pointer to the callback. Can be NULL to unset.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpExecBefore (void (*handler) (DPId id));
  /**
   * @brief Sets a callback function to be executed when process execution
   * fails.
   *
   * The callback is invoked in the child process when execvp() fails or when
   * environment setup fails before process execution.
   *
   * @param[in] handler Function pointer to the callback. Can be NULL to unset.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpExecFail (void (*handler) (DPId id));
  /**
   * @brief Sets a callback function to be executed after process exits
   * normally.
   *
   * The callback is invoked in the parent process when a child process
   * terminates normally (via exit() rather than being killed by a signal).
   *
   * @param[in] handler Function pointer to the callback (receives process ID and exit code). Can be NULL to unset.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpExecAfterExit (void (*handler) (DPId id, DPInt code));
  /**
   * @brief Sets a callback function to be executed after process is terminated
   * by signal.
   *
   * The callback is invoked in the parent process when a child process
   * is terminated by a signal (rather than exiting normally).
   *
   * @param[in] handler Function pointer to the callback (receives process ID and signal number). Can be NULL to unset.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpExecAfterSig (void (*handler) (DPId id, DPInt sig));
  /**
   * @brief Gets a human-readable error message for the last error.
   *
   * This function retrieves the error string corresponding to the last error
   * that occurred in the library, including any additional detail information.
   *
   * @param[out] err Buffer to store the error message string.
   * @param[in] sz Size of the buffer in bytes.
   * @param[out] used Pointer to store the number of bytes written, if not NULL.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpGetErrorStr (DPChar *err, DPSize sz, DPSize *used);

  /**
   * @brief Initializes a process definition with the specified parameters.
   *
   * This function creates and initializes a process definition structure
   * with the provided name, executable path, working directory, arguments,
   * and environment variables. All string parameters are copied internally.
   *
   * @param[out] def Pointer to store the initialized process definition.
   * @param[in] name Process name (must start with letter, contain only alphanumeric characters, underscore, or hyphen).
   * @param[in] app Path to the executable file.
   * @param[in] dir Working directory for the process.
   * @param[in] args Null-terminated array of command-line arguments.
   * @param[in] envs Null-terminated array of environment variables in "KEY=VALUE" format.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcDefInit (DPProcDef *def, const DPChar *name,
                                 const DPChar *app, const DPChar *dir,
                                 const DPChar *const args[],
                                 const DPChar *const envs[]);
  /**
   * @brief Cleans up and destroys a process definition.
   *
   * This function deallocates all memory associated with the process
   * definition including the name, executable path, working directory,
   * arguments, and environment variables.
   *
   * @param[in,out] def Pointer to the process definition to destroy (will be set to NULL after destruction).
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcDefDestroy (DPProcDef *def);
  /**
   * @brief Gets the process name from a process definition.
   *
   * @param[in] def Process definition to query.
   * @param[out] name Buffer to store the process name.
   * @param[in] sz Size of the buffer in bytes.
   * @param[out] used Pointer to store the number of bytes used, if not NULL.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcDefGetName (const DPProcDef *def, DPChar *name,
                                    DPSize sz, DPSize *used);
  /**
   * @brief Gets the executable path from a process definition.
   *
   * @param[in] def Process definition to query.
   * @param[out] app Buffer to store the executable path.
   * @param[in] sz Size of the buffer in bytes.
   * @param[out] used Pointer to store the number of bytes used, if not NULL.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcDefGetApp (const DPProcDef *def, DPChar *app,
                                   DPSize sz, DPSize *used);
  /**
   * @brief Gets the working directory from a process definition.
   *
   * @param[in] def Process definition to query.
   * @param[out] dir Buffer to store the working directory path.
   * @param[in] sz Size of the buffer in bytes.
   * @param[out] used Pointer to store the number of bytes used, if not NULL.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcDefGetDir (const DPProcDef *def, DPChar *dir,
                                   DPSize sz, DPSize *used);
  /**
   * @brief Gets the number of command-line arguments from a process
   * definition.
   *
   * @param[in] def Process definition to query.
   * @param[out] argc Pointer to store the argument count.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcDefGetArgc (const DPProcDef *def, DPSize *argc);
  /**
   * @brief Gets a specific command-line argument by index from a process
   * definition.
   *
   * @param[in] def Process definition to query.
   * @param[out] arg Buffer to store the argument string.
   * @param[in] idx Index of the argument to retrieve (0-based).
   * @param[in] sz Size of the buffer in bytes.
   * @param[out] used Pointer to store the number of bytes used, if not NULL.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number. Returns DP_EBOUND if idx is out of range.
   */
  DP_API DPStatus dpProcDefGetArg (const DPProcDef *def, DPChar *arg,
                                   DPByte idx, DPSize sz, DPSize *used);
  /**
   * @brief Gets the number of environment variables from a process definition.
   *
   * @param[in] def Process definition to query.
   * @param[out] argc Pointer to store the environment variable count.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcDefGetEnvc (const DPProcDef *def, DPSize *argc);
  /**
   * @brief Gets a specific environment variable by index from a process
   * definition.
   *
   * @param[in] def Process definition to query.
   * @param[out] env Buffer to store the environment variable in "KEY=VALUE" format.
   * @param[in] idx Index of the environment variable to retrieve (0-based).
   * @param[in] sz Size of the buffer in bytes.
   * @param[out] used Pointer to store the number of bytes used, if not NULL.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number. Returns DP_EBOUND if idx is out of range.
   */
  DP_API DPStatus dpProcDefGetEnv (const DPProcDef *def, DPChar *env,
                                   DPByte idx, DPSize sz, DPSize *used);

  /**
   * @brief Initializes a process attributes structure with default values.
   *
   * This function allocates and initializes a process attributes structure
   * with default values: no memory limit, no log size limit, 1 log rotation
   * file, and no restart delay.
   *
   * @param[out] attr Pointer to store the initialized process attributes.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcAttrInit (DPProcAttr *attr);
  /**
   * @brief Cleans up and destroys a process attributes structure.
   *
   * This function deallocates all memory associated with the process
   * attributes structure.
   *
   * @param[in,out] attr Pointer to the process attributes to destroy (will be set to NULL after destruction).
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcAttrDestroy (DPProcAttr *attr);
  /**
   * @brief Sets the maximum memory limit for a process.
   *
   * @param[in,out] attr Process attributes to modify.
   * @param[in] mem Maximum memory limit in bytes. Set to 0 for no limit.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcAttrSetMaxMem (DPProcAttr *attr, DPQword mem);
  /**
   * @brief Gets the maximum memory limit for a process.
   *
   * @param[in] attr Process attributes to query.
   * @param[out] mem Pointer to store the maximum memory limit in bytes (0 indicates no limit).
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcAttrGetMaxMem (const DPProcAttr *attr, DPQword *mem);
  /**
   * @brief Sets the maximum log file size for a process.
   *
   * @param[in,out] attr Process attributes to modify.
   * @param[in] sz Maximum log file size in bytes. Set to 0 for no limit.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcAttrSetLogMaxSize (DPProcAttr *attr, DPQword sz);
  /**
   * @brief Gets the maximum log file size for a process.
   *
   * @param[in] attr Process attributes to query.
   * @param[out] sz Pointer to store the maximum log file size in bytes (0 indicates no limit).
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcAttrGetLogMaxSize (const DPProcAttr *attr,
                                           DPQword *sz);
  /**
   * @brief Sets the number of log rotation files for a process.
   *
   * @param[in,out] attr Process attributes to modify.
   * @param[in] rot Number of log rotation files to keep.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcAttrSetLogRot (DPProcAttr *attr, DPByte rot);
  /**
   * @brief Gets the number of log rotation files for a process.
   *
   * @param[in] attr Process attributes to query.
   * @param[out] rot Pointer to store the number of log rotation files.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcAttrGetLogRot (const DPProcAttr *attr, DPByte *rot);
  /**
   * @brief Sets the restart delay in seconds for a process.
   *
   * This delay is applied before restarting a process that has exited
   * or been terminated, providing a cooling-off period to prevent
   * rapid restart loops.
   *
   * @param[in,out] attr Process attributes to modify.
   * @param[in] sec Restart delay in seconds. Set to 0 for no delay.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcAttrSetRestartSec (DPProcAttr *attr, DPDword sec);
  /**
   * @brief Gets the restart delay in seconds for a process.
   *
   * @param[in] attr Process attributes to query.
   * @param[out] sec Pointer to store the restart delay in seconds (0 indicates no delay).
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcAttrGetRestartSec (const DPProcAttr *attr,
                                           DPDword *sec);

  /**
   * @brief Registers a new process into the process manager.
   *
   * @param[in,out] id Process identifier assigned during registration. It is
   * possible to provide a positive number (including zero) as a suggestion. If
   * such an identifier is already allocated, the next free identifier higher
   * than the provided one is utilized.
   *
   * @param[in] def Process definition. An internal clone is created during registration,
   *               so the provided structure must still be cleaned up using dpProcDefDestroy().
   *
   * @param[in] attr Process attributes. An internal clone is created during registration,
   *                so the provided structure must still be cleaned up using dpProcAttrDestroy().
   *
   * @return On success, the function return 0; on error, it returns a negative
   * error number.
   */
  DP_API DPStatus dpProcReg (DPId *id, const DPProcDef *def,
                             const DPProcAttr *attr);
  /**
   * @brief Unregisters a process from the process manager.
   *
   * This function removes a process from the process registry. The process
   * must be in a stopped or errored state before it can be unregistered.
   *
   * @param[in] id Process identifier to unregister.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number. Returns DP_ENOT_STOPPED if process is still
   * running.
   */
  DP_API DPStatus dpProcUnreg (DPId id);
  /**
   * @brief Starts a registered process.
   *
   * This function schedules a registered process to be started. The actual
   * process execution occurs during the next dpTick() call. If the process
   * is already running, this function has no effect.
   *
   * @param[in] id Process identifier to start.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcStart (DPId id);
  /**
   * @brief Stops a running process.
   *
   * This function schedules a running process to be stopped by sending
   * SIGTERM signal. The actual termination occurs during the next dpTick()
   * call. If the process is already stopped, this function has no effect.
   *
   * @param[in] id Process identifier to stop.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcStop (DPId id);
  /**
   * @brief Restarts a process (stops then starts it).
   *
   * This function schedules a process to be restarted. If the process is
   * currently running, it will be stopped first, then started again after
   * the configured restart delay.
   *
   * @param[in] id Process identifier to restart.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcRestart (DPId id);
  /**
   * @brief Gets a copy of the process definition for a registered process.
   *
   * This function creates a copy of the process definition (name, executable,
   * working directory, arguments, environment variables) for the specified
   * process. The caller is responsible for cleaning up the returned
   * definition.
   *
   * @param[out] def Pointer to store the copied process definition.
   * @param[in] id Process identifier to query.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcGetDef (DPProcDef *def, DPId id);
  /**
   * @brief Gets a copy of the process attributes for a registered process.
   *
   * This function creates a copy of the process attributes (memory limits,
   * log settings, restart delay) for the specified process. The caller is
   * responsible for cleaning up the returned attributes.
   *
   * @param[out] attr Pointer to store the copied process attributes.
   * @param[in] id Process identifier to query.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcGetAttr (DPProcAttr *attr, DPId id);
  /**
   * @brief Gets runtime information for a registered process.
   *
   * This function retrieves current runtime information about a process
   * including its state, restart count, and process ID (if running).
   * The caller is responsible for cleaning up the returned info structure.
   *
   * @param[out] info Pointer to store the process runtime information.
   * @param[in] id Process identifier to query.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcGetInfo (DPProcInfo *info, DPId id);
  /**
   * @brief Checks if a process identifier is registered.
   *
   * This function tests whether the specified process ID exists in the
   * process registry.
   *
   * @param[in] id Process identifier to check.
   *
   * @return DP_OK if the process exists, DP_BREAK if it does not exist,
   * or a negative error number on error.
   */
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
  /**
   * @brief Initializes an iterator for traversing registered processes.
   *
   * This function initializes an iterator that can be used with
   * dpProcIterNext() to traverse all registered processes in the process
   * registry.
   *
   * @param[out] iter Pointer to the iterator to initialize.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcIter (DPProcIter *iter);
  /**
   * @brief Gets the next process identifier from the iterator.
   *
   * This function advances the iterator and retrieves the next process ID
   * from the process registry. Process IDs are returned in ascending order.
   *
   * @param[in,out] iter Iterator to advance.
   * @param[out] next Pointer to store the next process identifier, if not NULL.
   *
   * @return DP_OK if a next process was found, DP_BREAK if no more processes
   * are available, or a negative error number on error.
   */
  DP_API DPStatus dpProcIterNext (DPProcIter *iter, DPId *next);
  /**
   * @brief Loads process definitions from an INI configuration file.
   *
   * This function parses an INI file and registers all process definitions
   * found in the file. Each process is defined in its own section with
   * properties like name, app, dir, args, and various attributes.
   *
   * @param[in] ini Path to the INI configuration file to load.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number. Returns DP_ECONF for configuration file errors.
   */
  DP_API DPStatus dpProcLoad (const DPChar *ini);
  /**
   * @brief Saves all registered process definitions to an INI configuration
   * file.
   *
   * This function writes all currently registered processes and their
   * definitions and attributes to an INI file. Each process is saved
   * in its own section.
   *
   * @param[in] ini Path to the INI configuration file to create/overwrite.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcSave (const DPChar *ini);

  /**
   * @brief Cleans up and destroys a process info structure.
   *
   * This function deallocates memory associated with a process info
   * structure that was obtained from dpProcGetInfo().
   *
   * @param[in,out] info Pointer to the process info structure to destroy (will be set to NULL after destruction).
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcInfoDestroy (DPProcInfo *info);
  /**
   * @brief Gets the current state of a process.
   *
   * This function retrieves the current operational state of a process
   * (e.g., running, stopped, exited, etc.) from a process info structure.
   *
   * @param[in] info Process info structure to query.
   * @param[out] state Pointer to store the current process state.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcInfoGetState (const DPProcInfo *info,
                                      DPProcState *state);
  /**
   * @brief Gets the restart count for a process.
   *
   * This function retrieves the number of times a process has been
   * automatically restarted due to unexpected exits or signals.
   *
   * @param[in] info Process info structure to query.
   * @param[out] restarts Pointer to store the restart count.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcInfoGetRestarts (const DPProcInfo *info,
                                         DPDword *restarts);
  /**
   * @brief Gets the process ID (PID) of a running process.
   *
   * This function retrieves the operating system process ID for a
   * currently running process. If the process is not running, the
   * returned PID may be invalid.
   *
   * @param[in] info Process info structure to query.
   * @param[out] pid Pointer to store the process ID.
   *
   * @return On success, the function returns 0; on error, it returns a
   * negative error number.
   */
  DP_API DPStatus dpProcInfoGetPid (const DPProcInfo *info, DPPid *pid);

#ifdef __cplusplus
}
#endif

#endif
