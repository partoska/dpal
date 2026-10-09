# @partoska/dpal

Daemon Pal — a compact user-space tool for process management. Supervise,
restart, and persist long-running processes from your terminal.

[![npm version](https://img.shields.io/npm/v/@partoska/dpal.svg)](https://www.npmjs.com/package/@partoska/dpal)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](https://github.com/partoska/dpal/blob/main/LICENSE.txt)

## What is this?

`dpal` runs in two modes:

- **Control process** (`dpal -C`) — a small daemon that owns and supervises
  your processes, restarting them as configured.
- **Command executor** (default) — a short-lived client that tells a running
  control process what to do: add, start, stop, restart, list, delete,
  import/export, or persist processes.

**Key features:**

- Process lifecycle management with automatic startup and restarts.
- Process persistence across control process restarts.
- Individual and batch (`-a`) process control.
- Import/export of process lists via INI files.
- Per-process memory limits, log rotation, and restart intervals.
- Isolated working directories, so multiple control processes can coexist.
- Works on macOS and Linux (x64, arm64, armhf).

## Install

```bash
npm install -g @partoska/dpal
```

This installs a small launcher plus the prebuilt native binary for your
platform, selected automatically. No compiler or build tools are required — see
[How this package works](#how-this-package-works) for the details.

### Supported platforms

| OS    | Architectures              |
| ----- | -------------------------- |
| macOS | Intel & Apple Silicon      |
| Linux | x64, arm64, armhf (ARMv6+) |

Windows is not supported (dpal relies on POSIX process control and SysV IPC).

## Quick Start

```bash
dpal -C -a &                              # Start the control process.
dpal -c /usr/bin/python3 -m http.server   # Add a process.
dpal -l -a                                # List all processes.
dpal -s python3                           # Start it by name.
dpal -p                                   # Persist the list for next time.
dpal -k                                   # Shut the control process down.
```

Run `dpal -h` at any time for full usage.

## Usage

### Control process mode

```bash
dpal -C [-D workdir] [-f] [-i procs.ini] [-a]
```

| Flag | Long           | Meaning                                                |
| ---- | -------------- | ------------------------------------------------------ |
| `-C` | `--control`    | Start the control process.                             |
| `-D` | `--dir`        | Working directory for this control process.            |
| `-f` | `--force`      | Start even if another one holds the workdir (recovery).|
| `-i` | `--ini`        | Load a process list from an INI file at startup.       |
| `-a` | `--auto-start` | Automatically start every process in the list.         |

### Command executor mode

```bash
dpal [-D workdir] [-f] COMMAND [NAME|ID ...|-a]
```

| Flag | Long        | Meaning                                              |
| ---- | ----------- | ---------------------------------------------------- |
| `-l` | `--list`    | List processes.                                      |
| `-s` | `--start`   | Start process(es).                                   |
| `-t` | `--stop`    | Stop process(es) (SIGTERM).                          |
| `-r` | `--restart` | Stop and start process(es).                          |
| `-c` | `--create`  | Add a process from `EXEC [args...]`.                 |
| `-d` | `--delete`  | Remove process(es) from the list.                    |
| `-i` | `--import`  | Import a process list from an INI file.              |
| `-e` | `--export`  | Export the process list to an INI file.              |
| `-p` | `--persist` | Save the current list so it reloads on next start.   |
| `-k` | `--kill`    | Shut down the control process and all its processes. |

## Configuration

The working directory is resolved in this order: `-D/--dir`, `$DPAL_HOME`,
`$HOME/.dpal`, then the current directory. Use the same `-D` for the control
process and every command aimed at it.

Process lists use a simple INI format:

```ini
[web]
App=/usr/bin/python3
Dir=/srv/www
Arg=python3
Arg=-m
Arg=http.server
; Restart 5 seconds after the process exits.
RestartSec=5
; Rotate logs at 1 MiB, keeping 3 rotated files.
LogMaxSize=1048576
LogRotation=3
```

## How this package works

`@partoska/dpal` follows the per-platform `optionalDependencies` model. The
package you install ships only a tiny Node launcher; the actual native binary
comes from one of these platform packages, and npm installs only the one
matching your OS and CPU:

- `@partoska/dpal-darwin` — macOS universal (Intel + Apple Silicon)
- `@partoska/dpal-linux-x64`
- `@partoska/dpal-linux-arm64`
- `@partoska/dpal-linux-arm` — armhf (ARMv6+)

When you run `dpal`, the launcher resolves the matching binary and, on Node
22.15 or newer, replaces itself with it (`execve`), so the control process
owns its PID and receives signals directly. On older Node versions it runs
the binary as a child, forwarding signals and the exit status.

## Troubleshooting

### "Unsupported platform" or "could not find the native binary"

This usually means optional dependencies were skipped during install (for
example with `npm install --omit=optional`, or behind a strict CI mirror).
Reinstall with optional dependencies enabled:

```bash
npm install -g @partoska/dpal
```

### "No control process is running"

Executor commands talk to the control process bound to the working directory.
Make sure one is running (`dpal -C &`) and that both use the same `-D` or
`$DPAL_HOME`.

## License

MIT. Copyright (C) 2024 Fabrika Charvat s.r.o. Developed by the
[Partoska Laboratory](https://lab.partoska.com) team.

## Links

- **Project website:** <https://lab.partoska.com/dpal>
- **Source & issues:** <https://github.com/partoska/dpal>
