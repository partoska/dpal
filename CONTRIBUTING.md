# Contributing to dpal

Thank you for your interest in contributing! Here's everything you need to get started.

## Getting Started

### Prerequisites

- C99-capable compiler (GCC or Clang)
- CMake 3.13+
- Git

### Building

```bash
git clone https://github.com/partoska/dpal.git
cd dpal

# Debug build with sanitizers (recommended for development)
./build.sh -d -s

# The binary will be at: build/src/cli/dpal
```

### Build options

| Flag | Description |
|------|-------------|
| `-d` | Debug build (includes debug symbols) |
| `-s` | Enable address and undefined behavior sanitizers |
| `-t` | Build and register unit tests |
| `-v` | Verbose debug logging (`DEBUG_SLOW` macro) |
| `-j N` | Use N parallel build jobs |

### Running tests

```bash
./build.sh -t
./test.sh
```

## Submitting Changes

1. Fork the repository and create a branch from `main`.
2. Make your changes — keep commits focused and atomic.
3. Run a debug build with sanitizers (`./build.sh -d -s`) and verify it builds cleanly.
4. Run the tests (`./test.sh`) and verify they all pass.
5. Format your code with `clang-format` (configuration is in `.clang-format`).
6. Open a pull request with a clear description of what you changed and why.

## Code Style

- **Standard**: C99 with POSIX extensions (`-D_REENTRANT`)
- **Formatter**: `clang-format` (GNU style — run it before committing)
- **Naming**:
  - Public types: `PascalCase` with `Dp` prefix (e.g., `DpProcDef`, `DpAttr`)
  - Public functions: `camelCase` with `dp` prefix (e.g., `dpProcReg`, `dpProcStart`)
  - Macros: `UPPER_SNAKE_CASE` with `DP_` prefix
- **Warnings**: All warnings are treated as errors (`-Wall -Werror -Wextra`) — fix them, don't suppress them.
- **Error handling**: Functions return status codes defined in `dpaltypes.h`. Do not `exit()` from library code.

## Adding a Source File

The CMake build and the hand-written `Makefile` must stay in sync. When adding a
source file, list it in both:

1. `src/lib/CMakeLists.txt` or `src/cli/CMakeLists.txt`
2. `Makefile` — `LIB_SRCS` or `CLI_SRCS`

## Changing the Command Line

When adding or changing a command-line option, also update:

1. `src/cli/src/util.c` — the built-in help (`dpal -h`)
2. `README.md` — the usage sections
3. `npm/dpal/README.md` — the usage tables
4. `assets/installer/README.linux.txt` and `assets/installer/README.macos.txt`
5. `skills/dpal/SKILL.md` — the agent skill

## Reporting Issues

Please use the GitHub issue tracker. Bug reports are most useful when they include:

- The version of `dpal` (`dpal --version`)
- Your operating system and architecture
- The exact command you ran
- The full output, including any error messages

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE.txt).
