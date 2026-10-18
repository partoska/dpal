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

# End-to-end test of the list output formats (-l -F). Runs a control process
# in a temporary working directory and drives it with the given dpal binary.
#
# Usage: test_cli.sh <path-to-dpal>

DPAL="$1"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/dpal-cli-XXXXXX")"
CTRL=""

cleanup() {
    if [ -n "${CTRL}" ]; then
        "${DPAL}" -D "${WORK}" -k >/dev/null 2>&1 || kill "${CTRL}" 2>/dev/null
        wait "${CTRL}" 2>/dev/null
    fi
    rm -rf "${WORK}"
}
trap cleanup EXIT

fail() {
    echo "FAIL: $1"
    if [ -f "${WORK}/ctrl.log" ]; then
        echo "Control process log:"
        cat "${WORK}/ctrl.log"
    fi
    exit 1
}

# Runs an executor command and stores its exit code, stdout, and stderr.
# Debug builds also print debug messages to stdout, so those are dropped.
run() {
    "${DPAL}" -D "${WORK}" "$@" >"${WORK}/out" 2>"${WORK}/err"
    RC=$?
    OUT="$(grep -v -e '^debug: ' -e '^verbose: ' "${WORK}/out")"
    ERR="$(cat "${WORK}/err")"
}

expect() {
    local name="$1" rc="$2" out="$3"
    if [ "${RC}" -ne "${rc}" ]; then
        fail "${name}: exit code ${RC}, expected ${rc} (stderr: ${ERR})"
    fi
    if [ "${OUT}" != "${out}" ]; then
        printf 'Expected:\n%s\nActual:\n%s\n' "${out}" "${OUT}"
        fail "${name}: unexpected output"
    fi
}

expect_err() {
    local name="$1" msg="$2"
    case "${ERR}" in
        *"${msg}"*) ;;
        *) fail "${name}: stderr '${ERR}' does not contain '${msg}'" ;;
    esac
}

# Invalid usage is rejected before contacting the control process.
run -l -a -F xml
expect "unknown format" 1 "Usage: ${DPAL} -h"
expect_err "unknown format" "Invalid output format"

run -l -a --format JSON
expect "format is case sensitive" 1 "Usage: ${DPAL} -h"
expect_err "format is case sensitive" "Invalid output format"

run -s web -F json
expect "format without list" 1 "Usage: ${DPAL} -h"
expect_err "format without list" "Option --format requires --list"

run -L -a -F json
expect "format with log" 1 "Usage: ${DPAL} -h"
expect_err "format with log" "Option --format requires --list"

# Start the control process and wait until it replies.
"${DPAL}" -C -D "${WORK}" >"${WORK}/ctrl.log" 2>&1 &
CTRL=$!
for _ in 1 2 3 4 5 6 7 8 9 10; do
    run -l -a -F one
    [ "${RC}" -eq 0 ] && break
    sleep 0.5
done
expect "control process is up" 0 ""

# Two stopped processes keep the output deterministic (no PIDs).
run -c /bin/sleep 100
expect "create sleep" 0 "Create process"
run -c /bin/sleep 200
expect "create sleep-1" 0 "Create process"

ROW1='   1: sleep            STOPPED  [     -1 ]        0 ↻'
ROW2='   2: sleep-1          STOPPED  [     -1 ]        0 ↻'

run -l -a
expect "plain (default)" 0 "List all processes
${ROW1}
${ROW2}"

run -l -a --format plain
expect "plain (explicit)" 0 "List all processes
${ROW1}
${ROW2}"

run -l -a -F json
expect "json" 0 '[
  {"id":1,"name":"sleep","state":"stopped","pid":-1,"restarts":0},
  {"id":2,"name":"sleep-1","state":"stopped","pid":-1,"restarts":0}
]'

run -l -a --format=csv
expect "csv" 0 'id,name,state,pid,restarts
1,"sleep",stopped,-1,0
2,"sleep-1",stopped,-1,0'

run -l -a --format one
expect "one" 0 'sleep
sleep-1'

# Targets by name and ID print no status lines in the machine formats.
run -l sleep-1 1 -F csv
expect "csv targets" 0 'id,name,state,pid,restarts
2,"sleep-1",stopped,-1,0
1,"sleep",stopped,-1,0'

run -F json -l 2
expect "format before list" 0 '[
  {"id":2,"name":"sleep-1","state":"stopped","pid":-1,"restarts":0}
]'

run -l sleep-1
expect "plain target" 0 "List process: sleep-1
${ROW2}"

# A missing target fails, but the output stays well-formed.
run -l sleep nope -F json
expect "json missing target" 1 '[
  {"id":1,"name":"sleep","state":"stopped","pid":-1,"restarts":0}
]'
expect_err "json missing target" "Process not found"

run -l nope -F json
expect "json only missing target" 1 '[
]'

run -l nope -F csv
expect "csv only missing target" 1 'id,name,state,pid,restarts'

# The names printed by "one" are valid targets.
run -d $("${DPAL}" -D "${WORK}" -l -a -F one | grep -v -e '^debug: ' -e '^verbose: ')
expect "delete via one" 0 "Delete process: sleep
Delete process: sleep-1"

run -l -a -F json
expect "json empty" 0 '[
]'

echo "OK"
