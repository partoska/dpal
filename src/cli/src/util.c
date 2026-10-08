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

#include "dpaltypes.h"

#include "check.h"
#include "logger.h"
#include "plat.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static const DPChar DPAL_DIR[] = "/.dpal";

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

const DPChar *STATE_STOPPED_STR = "STOPPED";
const DPChar *STATE_RUNNING_STR = "RUNNING";

void
dpcPrepareWorkdir (DPChar *workdir, DPSize sz)
{
  DPSize homedirLen = dpcStrnlen (workdir, sz);
  CHECK_GENERIC (homedirLen < sz, "Work directory path is too long",
                 EXIT_WITH_FAILURE);
  if (homedirLen > 0)
    {
      return;
    }

  // Ok, no path via --dir arg specified. Try DPAL_HOME env variable first.
  const DPChar *env = dpcGetenv ("DPAL_HOME");
  if (env != NULL)
    {
      DPSize envLen = dpcStrnlen (env, sz);
      CHECK_GENERIC (envLen < sz, "The DPAL_HOME directory path is too long",
                     EXIT_WITH_FAILURE);
      dpcStrncat (workdir, env, sz - 1);

      return;
    }

  // Fallback to HOME directory of the current user.
  env = dpcGetenv ("HOME");
  if (env != NULL)
    {
      DPSize envLen = dpcStrnlen (env, sz);
      CHECK_GENERIC (envLen < sz - SIZEOF (DPAL_DIR),
                     "The HOME directory path is too long", EXIT_WITH_FAILURE);
      dpcStrncat (workdir, env, sz - 1);
      dpcStrncat (workdir, DPAL_DIR, CHARSMAX (DPAL_DIR));

      if (!dpcDirExists (workdir))
        {
          DPC_DEBUG ("Creating workdir: %s", workdir);

          CHECK_POSIX (dpcMkdir (workdir),
                       "Cannot create default work directory",
                       EXIT_WITH_FAILURE);
        }

      return;
    }

  // No HOME directory found, use the current working directory.
  CHECK_GENERIC (dpcGetcwd (workdir, sz) != NULL,
                 "Cannot determine the current work directory",
                 EXIT_WITH_FAILURE);
}

void
dpcPrintUsage (const DPChar *name)
{
  DPC_INFO ("Usage: %s -h", name);
}

void
dpcPrintLogo (void)
{
  DPC_INFO ("");
  DPC_INFO ("   ___                                ___         __");
  DPC_INFO ("  / _ \\ ___ _ ___  __ _  ___   ___   / _ \\ ___ _ / /");
  DPC_INFO (" / // // _ `// -_)/  ' \\/ _ \\ / _ \\ / ___// _ `// /");
  DPC_INFO ("/____/ \\_,_/ \\__//_/_/_/\\___//_//_//_/    \\_,_//_/");
  DPC_INFO ("");
  DPC_INFO ("Daemon Pal v" DP_VERSION ", <https://lab.partoska.com/dpal>");
  DPC_INFO ("Copyright (C) 2024 Fabrika Charvat s.r.o. All rights reserved.");
  DPC_INFO ("");
}

void
dpcPrintHelp (const DPChar *name)
{
  dpcPrintLogo ();

  DPC_INFO ("The Daemon Pal is a compact tool for process control. It was");
  DPC_INFO (
      "designed to operate in two possible modes: (1) as a control process");
  DPC_INFO (
      "and (2) as a command executor. The usual workflow involves starting");
  DPC_INFO ("the control process first and then modifying its behavior using");
  DPC_INFO ("the executor. Thus, both modes have different argument options.");
  DPC_INFO ("");
  DPC_INFO ("Usage:");
  DPC_INFO ("  %s -C [-D workdir] [-f] [-i procs.ini] [-a]", name);
  DPC_INFO ("  %s [-D workdir] [-f] COMMAND [-a]", name);
  DPC_INFO ("  %s [-D workdir] [-f] -i | -e procs.ini", name);
  DPC_INFO ("  %s [-D workdir] [-f] -p | -k", name);
  DPC_INFO ("");
  DPC_INFO ("General options:");
  DPC_INFO ("  -h/--help        Prints this help message.");
  DPC_INFO ("  -v/--version     Prints version information.");
  DPC_INFO ("");
  DPC_INFO ("Control process mode options:");
  DPC_INFO ("  -C/--control     Starts control process.");
  DPC_INFO ("  -D/--dir         Specifies working directory.");
  DPC_INFO ("                   Default: ${DPAL_HOME} and ${HOME}/.dpal.");
  DPC_INFO (
      "  -f/--force       Forces the start of control process even when");
  DPC_INFO ("                   there is another one running in the same");
  DPC_INFO ("                   working directory. Can be used for recovery");
  DPC_INFO ("                   after errors.");
  DPC_INFO ("  -i/--ini         Utilizes a list of processes defined");
  DPC_INFO ("                   in the provided file.");
  DPC_INFO ("  -a/--auto-start  Automatically starts each process");
  DPC_INFO ("                   in the current list.");
  DPC_INFO ("");
  DPC_INFO ("Command executor mode options:");
  DPC_INFO ("  -D/--dir         Specifies working directory of the control ");
  DPC_INFO ("                   process.");
  DPC_INFO ("                   Default: ${DPAL_HOME} and ${HOME}/.dpal.");
  DPC_INFO (
      "  -f/--force       Forces the start of command process even when");
  DPC_INFO ("                   there is another one running in the same");
  DPC_INFO ("                   working directory. Can be used for recovery");
  DPC_INFO ("                   after errors.");
  DPC_INFO ("  -i/--import      Imports a list of processes from the file.");
  DPC_INFO ("  -e/--export      Exports a list of processes to the file.");
  DPC_INFO ("  -p/--persist     Permanently saves the current process list");
  DPC_INFO ("                   into workdir so the list is reloaded when");
  DPC_INFO ("                   the control process is launched again.");
  DPC_INFO ("  -k/--kill        Shuts down the control process.");
  DPC_INFO ("                   All controled processes are stopped.");
  DPC_INFO ("  -a/--all         Applies the COMMAND to ALL processes.");
  DPC_INFO ("");
  DPC_INFO (
      "  COMMAND(s):      Processes targeted by COMMAND can be specified");
  DPC_INFO (
      "                   either via NAME or ID (unless --all is used).");
  DPC_INFO ("  -l/--list        Lists all available processes.");
  DPC_INFO ("  -s/--start       Starts a process.");
  DPC_INFO ("  -t/--stop        Terminates a process by sending SIGTERM.");
  DPC_INFO ("  -r/--restart     Terminates and starts a process again.");
  DPC_INFO ("  -d/--delete      Removes a process from the list.");
  DPC_INFO ("");
}
