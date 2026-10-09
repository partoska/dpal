---
name: dpal
description: Use the Daemon Pal (dpal) CLI to manage process lifecycles — start a control process, then add, start, stop, restart, list, delete, import/export, and persist managed processes. Use when the user wants to run, supervise, or script long-running processes with dpal.
license: MIT
metadata:
  author: Partoska Laboratory
  version: "1.0.1"
---

# Daemon Pal (dpal)

Daemon Pal is a compact process supervisor. It runs in two modes:

- **Control process mode** (`-C`): the long-lived daemon that owns and supervises child processes.
- **Command executor mode** (default): a short-lived client that sends a single command to a running control process.

The usual workflow is: start one control process, then issue executor commands against it.

## Mental model

- A **control process** is scoped to a **working directory**. All executor commands target the control process found via that working directory, so the same `-D` must be used for the daemon and its clients.
- Working directory is resolved in this order: `-D/--dir` → `$DPAL_HOME` → `$HOME/.dpal` → current directory.
- **Processes** are identified by **NAME** or numeric **ID**. Most commands take one or more targets, or `-a/--all`.
- IPC is via SysV message queues keyed off the working directory. "No control process is running" means no daemon is bound to that workdir.
- The control process `chdir`s into its working directory. Each child then switches to its own `Dir=` before exec, so a relative `App=` (e.g. `./server`) resolves against `Dir`, and a bare name (e.g. `node`) is looked up on the control process's `PATH`.
- **IDs** are unique and never clash. An INI entry without `Id=` defaults to `0`. If the requested ID is taken, the lowest free ID ≥ it is used instead. Processes added with `-c` get the highest ID + 1. `-p`/`-e` write the assigned IDs back out, so they stay stable across restarts.

## Installation

Daemon Pal is available for macOS and Linux (no Windows binaries yet). If the environment cannot run installers, using portable binaries or building from source are the fallbacks.

### Step 1: Install dpal

#### Option A: Package manager (recommended)

Homebrew on macOS or Linux (x86_64 / arm64):

```bash
brew install partoska/tap/dpal
```

npm on macOS or Linux (x64, arm64, armhf):

```bash
npm install -g @partoska/dpal
```

Prefer npm when Node.js is already available; prefer Homebrew when it is already the system's package manager.

#### Option B: Platform-specific binary from GitHub Releases

Download the latest release from GitHub:

**https://github.com/partoska/dpal/releases**

If network access is available and you want exact commands, resolve the latest release via the GitHub Releases API rather than inventing versioned URLs:

```bash
curl -L https://api.github.com/repos/partoska/dpal/releases/latest
```

Select the installer from `assets[].name`, use its `browser_download_url`, and verify against its `digest` when available. Do **not** hard-code a version or construct a release URL by guessing.

#### Choose the right asset

| Platform | Preferred interactive install | Portable/headless fallback |
|---|---|---|
| macOS | `dpal_<version>_darwin_universal.pkg` | `dpal_<version>_darwin_universal.tar.gz` or bare `dpal_<version>_darwin_universal` |
| Debian/Ubuntu amd64 | `dpal_<version>_linux_amd64.deb` | `dpal_<version>_linux_amd64.tar.gz` or bare `dpal_<version>_linux_amd64` |
| Debian/Ubuntu arm64 | `dpal_<version>_linux_arm64.deb` | `dpal_<version>_linux_arm64.tar.gz` or bare `dpal_<version>_linux_arm64` |
| Fedora/RHEL amd64 | `dpal_<version>_linux_amd64.rpm` | `dpal_<version>_linux_amd64.tar.gz` or bare `dpal_<version>_linux_amd64` |
| Fedora/RHEL arm64 | `dpal_<version>_linux_arm64.rpm` | `dpal_<version>_linux_arm64.tar.gz` or bare `dpal_<version>_linux_arm64` |
| Debian/Ubuntu armhf | `dpal_<version>_linux_armhf.deb` | `dpal_<version>_linux_armhf.tar.gz` or bare `dpal_<version>_linux_armhf` |

Use `uname -s` for OS and `uname -m` for CPU architecture. Map `x86_64` to `amd64`; map `aarch64` or `arm64` to `arm64`; map `armv6l` or `armv7l` to `armhf`. On Linux, inspect `/etc/os-release` to choose `.deb` for Debian/Ubuntu-family systems and `.rpm` for Fedora/RHEL-family systems.

#### Install examples

macOS package:

```bash
sudo installer -pkg ./dpal_<version>_darwin_universal.pkg -target /
```

Debian/Ubuntu:

```bash
sudo apt install ./dpal_<version>_linux_amd64.deb
```

Fedora/RHEL:

```bash
sudo dnf install ./dpal_<version>_linux_amd64.rpm
```

Portable tarball (Linux or macOS). The archive extracts into a `dpal_<version>_<platform>/` directory, and the binary sits under `bin/` on Linux and under `usr/local/bin/` on macOS, so locate it instead of assuming a path:

```bash
tar -xzf ./dpal_<version>_<platform>.tar.gz
mkdir -p ~/.local/bin
install -m 755 "$(find ./dpal_<version>_<platform> -type f -path '*/bin/dpal')" ~/.local/bin/dpal
```

Portable bare binary:

```bash
chmod +x ./dpal_<version>_<platform>
mkdir -p ~/.local/bin
mv ./dpal_<version>_<platform> ~/.local/bin/dpal
```

Make sure `~/.local/bin` is on `PATH` when using portable installs.

#### Verify downloaded assets

When the release API provides a `digest` such as `sha256:<hex>`, verify the downloaded file before installing:

```bash
shasum -a 256 ./dpal_<version>_<platform>.<ext>
```

Compare the output hash to the asset digest from the release API.

### Step 2: Verify the install

```bash
dpal -v
```

## Quick reference

```
dpal -C [-D workdir] [-f] [-i procs.ini] [-a]   # start control process (daemon)
dpal [-D workdir] [-f] COMMAND [targets...|-a]  # send a command to the daemon
dpal -h                                         # full help
dpal -v                                         # print version
```

Exactly **one** command per executor invocation. `-D`, `-f`, and `-a` are modifiers, not commands.

### Control process options (`-C`)

| Flag | Long | Meaning |
|------|------|---------|
| `-C` | `--control` | Start the control process (daemon). |
| `-D` | `--dir` | Working directory for this daemon. |
| `-f` | `--force` | Start even if another daemon holds the workdir (recovery). |
| `-i` | `--ini` | Load process list from an INI file at startup (relative paths resolve against the current directory; without `-i`, `<workdir>/dpal.ini` is loaded, created empty if missing). |
| `-a` | `--auto-start` | Auto-start every process in the loaded list. |

### Executor commands

| Flag | Long | Takes | Meaning |
|------|------|-------|---------|
| `-l` | `--list` | targets / `-a` | List processes. |
| `-s` | `--start` | targets / `-a` | Start process(es). |
| `-t` | `--stop` | targets / `-a` | Stop process(es) with SIGTERM. |
| `-r` | `--restart` | targets / `-a` | Stop then start process(es). |
| `-c` | `--create` | `EXEC [args...]` | Add a new process definition (stopped; start it with `-s`). |
| `-d` | `--delete` | targets / `-a` | Remove stopped process(es) from the list. |
| `-i` | `--import` | `FILE` | Import a process list from an INI file. |
| `-e` | `--export` | `FILE` | Export the current process list to an INI file. |
| `-p` | `--persist` | — | Save current list into workdir so it reloads on next daemon start. |
| `-k` | `--kill` | — | Shut down the control process (stops all its processes). |

Executor-only modifiers: `-D/--dir`, `-f/--force`, `-a/--all`.

### Exit codes

The executor exits `1` on usage errors, when no control process is running, when the control process reports an error (e.g. `Process not found`, `Cannot remove process`), or when it does not reply. With several targets, every target is still processed, and the exit code is `1` if any of them failed. The control process (`-C`) exits `1` if it cannot start, for example because another daemon holds the workdir or the INI file is missing or invalid.

### Reading `-l` output

```
List all processes
   1: ticker           RUNNING  [   1441 ]        1 ↻
   3: crasher          RUNNING  [     -1 ]        6 ↻
   4: sleep            STOPPED  [     -1 ]        0 ↻
```

The columns are ID, NAME, state, PID, and restart count (`↻`, which also counts manual `-r` restarts). `RUNNING` with PID `-1` means the process exited and is waiting `RestartSec` before it is respawned. A process that keeps crashing shows a growing `↻` count. `-l` also takes targets (`dpal -l web 3`).

### Logs

Each process gets its stdout and stderr captured into `<workdir>/log/<NAME>-out.log` and `<workdir>/log/<NAME>-err.log`. When `LogMaxSize` is set, these files rotate to `.0`, `.1`, … (up to `LogRotation` copies). To monitor a process, tail these files, e.g. `tail -f ~/.dpal/log/web-out.log`. dpal has no built-in log command.

## Common workflows

**Start the daemon and auto-start everything from a config:**
```bash
dpal -C -i procs.ini -a &
```

**Use an isolated working directory** (so multiple daemons can coexist):
```bash
dpal -C -D /srv/myapp/.dpal -a &
dpal -D /srv/myapp/.dpal -l -a       # query that specific daemon
```

**Add a process, then start it by name:**
```bash
dpal -c /usr/bin/nginx -g "daemon off;"   # everything after EXEC is the new process's argv
dpal -s nginx
```

The NAME is derived from the basename of EXEC. Characters other than letters, digits, `_` and `-` become `_`, and a non-letter first character becomes `p` (`./srv.sh` → `srv_sh`). On a name collision, `-1`, `-2`, … is appended. The process gets `Dir=` set to the directory you ran `dpal -c` from, and no `Env=` or other attributes. To set those, write an INI file and import it with `-i`, or `-e` export, edit, delete, and re-import.

**Inspect, control, and tear down:**
```bash
dpal -l -a            # list all processes (NAME, ID, state)
dpal -t web           # stop the "web" process
dpal -r 10            # restart process with ID 10
dpal -t -a            # stop all processes
dpal -k               # shut down the control process entirely
```

**Snapshot and restore configuration:**
```bash
dpal -e backup.ini    # export current list
dpal -p               # persist current list into workdir (survives daemon restart)
dpal -i more.ini      # import additional processes at runtime
```

**Recover a stuck daemon/queue:**
```bash
dpal -C -f &          # force-start, reclaiming the workdir
dpal -f -l -a         # force a client command past a hung queue
```

## INI process definition format

Each `[section]` is a process NAME. Import with `-i` (daemon `--ini` or executor `--import`).

```ini
; Comments must be on their own line; inline "; ..." becomes part of the value.
[ls]
; Numeric ID (optional; for targeting by ID).
Id=0
; Executable path and working directory for the child (both required).
App=/bin/ls
Dir=/opt
; argv[0] (conventionally the program name), then one Arg= line per argument.
Arg=ls
Arg=-l
Arg=-h
; Max log size before rotation, rotated logs to keep.
LogMaxSize=64
LogRotation=3
; Delay in seconds before restart.
RestartSec=10

[printenv]
App=printenv
Dir=/opt
Arg=printenv
Arg=ENV
; Environment variable for the child.
Env=(Name=ENV,Value=STAGE)
```

See `example/ecosystem.ini` in the repo for a working sample.

## Guidance for the agent

- Before issuing executor commands, confirm a control process is running for the intended workdir; otherwise commands fail with "No Daemon Pal control process is running." Start one with `-C` first.
- Always pass the **same `-D`** to the daemon and every executor command targeting it.
- Don't combine two commands in one invocation — it errors with "Too many commands."
- `-c/--create` consumes the rest of the command line as the new process's `EXEC` and argv; put it last.
- `-d` refuses to delete a running process ("Cannot remove process"). Stop it with `-t` first.
- `-p` writes `<workdir>/dpal.ini`. The next control process started without `-i` loads it, and with `-a` auto-starts it.
- To monitor tasks, poll `dpal -l -a` (state, PID, restart count) and tail `<workdir>/log/<NAME>-{out,err}.log`. Don't assume a command succeeded unless its exit code is `0`.
- `-k` is destructive (stops all supervised processes and the daemon). `-t -a` stops processes but leaves the daemon up. Confirm intent before running either against a live system.
- `-f/--force` is a recovery escape hatch (reclaims workdir / clears hung queues); don't use it routinely.
- Run `dpal -h` to print the authoritative built-in help if flags seem to differ from this summary.
