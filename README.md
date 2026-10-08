<p align="center">
  <img src="img/logo-mark.svg" width="160" alt="Daemon Pal logo">
</p>

# Daemon Pal

[![Version](https://img.shields.io/badge/version-1.3.0-blue.svg)](https://github.com/partoska/dpal/releases/tag/v1.3.0)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE.txt)
[![Build](https://github.com/partoska/dpal/actions/workflows/build.yml/badge.svg)](https://github.com/partoska/dpal/actions/workflows/build.yml)
[![npm](https://img.shields.io/npm/v/@partoska/dpal.svg)](https://www.npmjs.com/package/@partoska/dpal)

Daemon Pal is a compact tool for process control designed to operate in two modes: as a control process and as a command executor. The control process manages the lifecycle of other processes, while the executor allows you to modify the behavior of the control process.

## Features

- Dual-mode operation (control process and command executor).
- Process lifecycle management.
- Configurable working directory.
- Import/export process lists.
- Automatic process startup.
- Process persistence across restarts.
- Individual and batch process control.

## Supported Platforms

| OS    | Architectures                                            |
| ----- | -------------------------------------------------------- |
| macOS | Universal (Intel & Apple Silicon), macOS 11+             |
| Linux | amd64, arm64, armhf (ARMv6+, including every Raspberry Pi) |

## Installation

### Homebrew (macOS, Linux amd64/arm64)

```bash
brew install partoska/tap/dpal
```

### npm (macOS, Linux amd64/arm64/armhf)

```bash
npm install -g @partoska/dpal
```

### Packages and binaries

Every [GitHub release](https://github.com/partoska/dpal/releases) ships a macOS
`.pkg`, Linux `.deb` (amd64, arm64, armhf) and `.rpm` (amd64, arm64) packages,
`.tar.gz` archives, bare binaries, and a `SHA256SUMS` file. For example, on
Raspberry Pi OS (32-bit):

```bash
sudo apt install ./dpal_<version>_linux_armhf.deb
```

### Build from source

Prerequisites: CMake 3.13+, a C99 compiler (gcc/clang), and a POSIX-compliant
operating system.

```bash
git clone https://github.com/partoska/dpal && cd dpal
./build.sh
sudo ./install.sh
```

## Usage

### General Options

- `-h, --help`: Print help message.
- `-v, --version`: Print version information.

### Control Process Mode

Start the control process first to manage other processes:

```bash
dpal -C [-D workdir] [-f] [-i procs.ini] [-a]
```

Options:
- `-C, --control`: Starts control process.
- `-D, --dir`: Specifies working directory (Default: `${DPAL_HOME}` and `${HOME}/.dpal`).
- `-f, --force`: Forces start even when another process is running (recovery option).
- `-i, --ini`: Utilizes processes defined in the provided file.
- `-a, --auto-start`: Automatically starts each process in the current list.

### Command Executor Mode

Modify the behavior of a running control process. Processes can be specified by NAME or ID (unless -a/--all is used).

```bash
dpal [-D workdir] [-f] COMMAND [-a]
dpal [-D workdir] [-f] -i | -e procs.ini
dpal [-D workdir] [-f] -c EXEC [args...]
dpal [-D workdir] [-f] -p | -k
```

Options:
- `-D, --dir`: Specifies control process working directory.
- `-f, --force`: Forces command execution despite other running processes.
- `-i, --import`: Imports process list from file.
- `-e, --export`: Exports process list to file.
- `-p, --persist`: Saves current process list permanently.
- `-k, --kill`: Shuts down the control process (stops all controlled processes).
- `-a, --all`: Applies command to ALL processes.
- `-l, --list`: Lists all available processes.
- `-s, --start`: Starts a process.
- `-t, --stop`: Terminates a process (sends SIGTERM).
- `-r, --restart`: Terminates and restarts a process.
- `-c, --create`: Adds a new process from EXEC path and arguments.
- `-d, --delete`: Removes a process from the list.

## Configuration

Daemon Pal looks for its working directory in the following locations:

1. `${DPAL_HOME}`
2. `${HOME}/.dpal`
3. Current working directory

Process definitions can be imported from and exported to INI files using the `-i` and `-e` options respectively.

## Examples

1. Start control process with automatic process startup:
```bash
dpal -C -a &
```

2. Import processes from configuration:
```bash
dpal -i procs.ini
```

3. List all managed processes:
```bash
dpal -l -a
```

4. Create a new process definition:
```bash
dpal -c /usr/bin/nginx -g "daemon off;"
```

5. Start a specific process:
```bash
dpal -s nginx
```

6. Stop all processes:
```bash
dpal -t -a
```

7. Stop the control process:
```bash
dpal -k
```

## Contributing

Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md). To report a
security issue, follow [SECURITY.md](SECURITY.md).

## License

This project is licensed under the [MIT License](LICENSE.txt). Third-party
notices are listed in [THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt).

## Contact

You can contact the author(s) via email at ask <at> partoska.com.

## Copyright

Copyright (C) 2024 Fabrika Charvat s.r.o. All rights reserved.
Developed by Partoska Laboratory team, &lt;<https://lab.partoska.com>&gt;
