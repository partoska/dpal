#!/bin/bash
#
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

usage() {
    echo "Usage: $0 [-d [-v]] [-s] [-t] [-j N] [-p] [-h]"
    echo "Options:"
    echo "  -d, --debug         Build in debug mode"
    echo "  -s, --sanitizers    Build with sanitizers"
    echo "  -t, --tests         Build and register unit tests"
    echo "  -v, --verbose       Verbose debug mode"
    echo "  -j N, --jobs N      Use N parallel jobs"
    echo "  -p, --pack          Create installation package"
    echo "  -h, --help          Show this help"
}

BUILD="Release"
SAN="OFF"
TESTS="OFF"
VERBOSE="OFF"
JOBS="--parallel 1"
PACK="OFF"

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -d|--debug)
            BUILD="Debug"
            shift
            ;;
        -s|--sanitizers)
            SAN="ON"
            shift
            ;;
        -t|--tests)
            TESTS="ON"
            shift
            ;;
        -v|--verbose)
            VERBOSE="ON"
            shift
            ;;
        -p|--pack)
            PACK="ON"
            shift
            ;;
        -j|--jobs)
            if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then
                JOBS="--parallel $2"
                shift 2
            else
                echo "Invalid option: -j requires a numeric argument"
                usage
                exit 1
            fi
            ;;
        *)
            echo "Invalid option: $1"
            usage
            exit 1
            ;;
    esac
done

cmake -S . -B ./build -DCMAKE_BUILD_TYPE=${BUILD} -DWITH_SANITIZERS=${SAN} -DWITH_VERBOSE_DEBUG=${VERBOSE} -DWITH_TESTS=${TESTS} -DCMAKE_VERBOSE_MAKEFILE=ON || exit 1
cmake --build ./build ${JOBS} || exit 1

if [[ "${PACK}" == "ON" ]]; then
    cpack --config ./build/CPackConfig.cmake || exit 1
    rm -rf ./_CPack_Packages
fi

exit 0
