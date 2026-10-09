# Daemon Pal - Compact user-space tool/library for process management.
# Copyright (C) 2024 Fabrika Charvat s.r.o. All rights reserved.
# Developed by Partoska Laboratory team, <https://lab.partoska.com>
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# You can contact the author(s) via email at ask <at> partoska.com.
#

# Project configuration.
PROJECT = dpal
LIBRARY = dplib
VERSION = 1.3.2
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

# Build configuration.
BUILD_TYPE ?= release
SANITIZERS ?= 0
VERBOSE_DEBUG ?= 0

# Directories.
SRCDIR = src
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj
BINDIR_BUILD = $(BUILDDIR)

# Compiler configuration.
CC ?= cc
AR ?= ar
ARFLAGS = rcs
STRIP ?= strip

# Library source files.
LIB_SRCS = $(SRCDIR)/lib/src/app/proc.c \
           $(SRCDIR)/lib/src/app/persist.c \
           $(SRCDIR)/lib/src/bus/init.c \
           $(SRCDIR)/lib/src/bus/registry.c \
           $(SRCDIR)/lib/src/core/error.c \
           $(SRCDIR)/lib/src/core/def.c \
           $(SRCDIR)/lib/src/core/attr.c \
           $(SRCDIR)/lib/src/core/info.c \
           $(SRCDIR)/lib/src/core/log.c \
           $(SRCDIR)/lib/src/plat/types.c \
           $(SRCDIR)/lib/src/plat/common.c \
           $(SRCDIR)/lib/src/plat/io.c \
           $(SRCDIR)/lib/src/plat/logger.c \
           $(SRCDIR)/lib/src/plat/alloc.c \
           $(SRCDIR)/lib/src/plat/str.c \
           $(SRCDIR)/lib/src/plat/env.c \
           $(SRCDIR)/lib/src/plat/select.c \
           $(SRCDIR)/lib/src/plat/filesys.c \
           $(SRCDIR)/lib/src/plat/fork.c \
           $(SRCDIR)/lib/src/plat/time.c

# CLI source files.
CLI_SRCS = $(SRCDIR)/cli/src/msg.c \
           $(SRCDIR)/cli/src/logger.c \
           $(SRCDIR)/cli/src/plat.c \
           $(SRCDIR)/cli/src/check.c \
           $(SRCDIR)/cli/src/util.c \
           $(SRCDIR)/cli/src/ctrl.c \
           $(SRCDIR)/cli/src/cmd.c \
           $(SRCDIR)/cli/src/entry.c

LIB_OBJS = $(LIB_SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
CLI_OBJS = $(CLI_SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJS = $(LIB_OBJS) $(CLI_OBJS)

# Include directories (PUBLIC includes from dplib propagate to every target).
INCLUDES = -I$(SRCDIR)/lib/inc \
           -I$(SRCDIR)/lib/src

# Compiler flags.
CFLAGS_COMMON = -std=gnu99 -D_REENTRANT -fno-strict-aliasing -pedantic \
                -Wall -Werror -Wextra -Werror=return-type -Werror=array-bounds

CFLAGS_DEBUG = -O0 -g3 -D_DEBUG=1
CFLAGS_RELEASE = -Os -g0 -DNDEBUG

# Build type selection.
ifeq ($(BUILD_TYPE),debug)
    CFLAGS_BUILD = $(CFLAGS_DEBUG)
else
    CFLAGS_BUILD = $(CFLAGS_RELEASE)
endif

# Verbose debug flag.
ifeq ($(VERBOSE_DEBUG),1)
    CFLAGS_BUILD += -DDEBUG_SLOW=1
endif

# Sanitizer flags.
ifeq ($(SANITIZERS),1)
    SANITIZER_FLAGS = -fno-omit-frame-pointer \
                      -fsanitize=undefined \
                      -fsanitize=address \
                      -fsanitize=float-cast-overflow \
                      -fsanitize-address-use-after-scope \
                      -fno-sanitize-recover

    # Add integer sanitizer for Clang.
    ifeq ($(shell $(CC) --version 2>/dev/null | grep -i clang),)
        # No Clang (e.g. GCC) - No integer sanitizer.
    else
        # Clang - Add integer sanitizer.
        SANITIZER_FLAGS += -fsanitize=integer
    endif

    CFLAGS_COMMON += $(SANITIZER_FLAGS)
    LDFLAGS += $(SANITIZER_FLAGS)
endif

# Automatic header dependency generation.
DEPFLAGS = -MMD -MP

# Complete CFLAGS.
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_BUILD) $(INCLUDES)

# Target artifacts.
LIBTARGET = $(BINDIR_BUILD)/lib$(LIBRARY).a
TARGET = $(BINDIR_BUILD)/$(PROJECT)

# Test configuration.
TESTDIR = test
TESTBINDIR = $(BUILDDIR)/test
TEST_CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_BUILD) -I$(SRCDIR)/lib/inc
TEST_BINS = $(TESTBINDIR)/test_simple \
            $(TESTBINDIR)/test_loop \
            $(TESTBINDIR)/test_advanced

# Aggregate dependency files.
DEPS = $(OBJS:.o=.d)

# Phony targets.
.PHONY: all clean install install-strip uninstall help debug release tests test

# Default target.
all: $(TARGET)

# Build executable (linked against the static library).
$(TARGET): $(CLI_OBJS) $(LIBTARGET) | $(BINDIR_BUILD)
	@echo "Linking $(PROJECT)..."
	$(CC) $(CLI_OBJS) $(LIBTARGET) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Archive static library (CMake add_library defaults to STATIC).
$(LIBTARGET): $(LIB_OBJS) | $(BINDIR_BUILD)
	@echo "Archiving $(LIBRARY)..."
	$(AR) $(ARFLAGS) $@ $(LIB_OBJS)
	@echo "Library complete: $@"

# Compile source files.
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Create build directories.
$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(BINDIR_BUILD):
	@mkdir -p $(BINDIR_BUILD)

$(TESTBINDIR):
	@mkdir -p $(TESTBINDIR)

# Build test binaries (each linked against the static library).
$(TESTBINDIR)/%: $(TESTDIR)/%.c $(LIBTARGET) | $(TESTBINDIR)
	@echo "Building test $*..."
	$(CC) $(TEST_CFLAGS) $< $(LIBTARGET) -o $@ $(LDFLAGS)

# Clean build artifacts.
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILDDIR)
	@echo "Clean complete."

# Install binary.
install: $(TARGET)
	@echo "Installing $(PROJECT) to $(BINDIR)..."
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(PROJECT)
	@echo "Installation complete."

# Install and strip binary.
install-strip: $(TARGET)
	@echo "Installing $(PROJECT) to $(BINDIR) (stripped)..."
	install -d $(BINDIR)
	install -m 755 -s $(TARGET) $(BINDIR)/$(PROJECT)
	@echo "Installation complete."

# Uninstall binary.
uninstall:
	@echo "Uninstalling $(PROJECT) from $(BINDIR)..."
	rm -f $(BINDIR)/$(PROJECT)
	@echo "Uninstall complete."

# Convenience targets for build types.
debug:
	@$(MAKE) BUILD_TYPE=debug

release:
	@$(MAKE) BUILD_TYPE=release

tests: $(TEST_BINS)

test: tests
	@failed=0; \
	for t in $(TEST_BINS); do \
	    $$t || failed=1; \
	done; \
	if [ $$failed -eq 0 ]; then echo "All tests passed."; else echo "Some tests FAILED."; exit 1; fi

# Help target.
help:
	@echo "Makefile for $(PROJECT) - Build targets:"
	@echo ""
	@echo "  make                        Build release version (default)."
	@echo "  make debug                  Build debug version."
	@echo "  make release                Build release version."
	@echo "  make clean                  Remove build artifacts."
	@echo "  make install                Install binary to $(BINDIR)."
	@echo "  make install-strip          Install and strip binary."
	@echo "  make uninstall              Remove installed binary."
	@echo "  make tests                  Build unit test binaries."
	@echo "  make test                   Build and run unit tests."
	@echo "  make help                   Show this help message."
	@echo ""
	@echo "Build options (set as environment variables or make arguments):"
	@echo ""
	@echo "  BUILD_TYPE=release|debug    Build type (default: release)."
	@echo "  SANITIZERS=0|1              Enable sanitizers (default: 0)."
	@echo "  VERBOSE_DEBUG=0|1           Enable verbose debug logging (default: 0)."
	@echo "  PREFIX=/path                Installation prefix (default: /usr/local)."
	@echo "  CC=compiler                 C compiler (default: cc)."
	@echo ""
	@echo "Examples:"
	@echo ""
	@echo "  make BUILD_TYPE=debug SANITIZERS=1"
	@echo "  make BUILD_TYPE=debug VERBOSE_DEBUG=1"
	@echo "  make PREFIX=/usr install"
	@echo "  make clean && make"
	@echo ""

# Include auto-generated header dependencies.
-include $(DEPS)
