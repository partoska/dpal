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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing -D_REENTRANT -Wall -Werror -Wextra -Werror=return-type -Werror=array-bounds -pedantic")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -D_DEBUG=1")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0 -DNDEBUG")

option(WITH_SANITIZERS "Use address and undefined sanitizers")

if(WITH_SANITIZERS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=address -fsanitize=float-cast-overflow -fsanitize-address-use-after-scope -fno-sanitize-recover")
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        # -fsanitize=integer is a Clang-only umbrella; GCC has no equivalent.
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=integer")
    endif()
endif()

option(WITH_VERBOSE_DEBUG "Extra verbose mode in Debug build")

if(WITH_VERBOSE_DEBUG)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_SLOW=1")
endif()

add_library(dplib

    # Public includes
    inc/dpal.h inc/dpaltypes.h

    # App layer
    src/app/proc.c src/app/proc.h src/app/persist.h src/app/persist.c

    # Business layer
    src/bus/init.h src/bus/init.c src/bus/registry.h src/bus/registry.c

    # Core layer
    src/core/error.h src/core/error.c
    src/core/def.h src/core/def.c src/core/attr.h src/core/attr.c src/core/info.h src/core/info.c
    src/core/log.h src/core/log.c

    # Platform layer
    src/plat/types.h src/plat/types.c src/plat/common.h src/plat/common.c
    src/plat/io.h src/plat/io.c src/plat/logger.h src/plat/logger.c
    src/plat/alloc.h src/plat/alloc.c src/plat/str.h src/plat/str.c
    src/plat/env.h src/plat/env.c src/plat/select.h src/plat/select.c src/plat/filesys.h src/plat/filesys.c
    src/plat/fork.h src/plat/fork.c src/plat/time.h src/plat/time.c)

target_include_directories(dplib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc")
target_include_directories(dplib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
