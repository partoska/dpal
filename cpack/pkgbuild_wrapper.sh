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

# Wrapper for pkgbuild that adjusts --root and --install-location so the
# package payload starts at /usr/local rather than /, avoiding the macOS 13+
# "trying to install content to the system volume" error that occurs when the
# payload contains a ./usr directory entry.

REAL_PKGBUILD=/usr/bin/pkgbuild
PREFIX="/usr/local"

args=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --root)
            args+=("--root" "${2}${PREFIX}")
            shift 2
            ;;
        --install-location)
            args+=("--install-location" "$PREFIX")
            shift 2
            ;;
        *)
            args+=("$1")
            shift
            ;;
    esac
done

exec "$REAL_PKGBUILD" "${args[@]}"
