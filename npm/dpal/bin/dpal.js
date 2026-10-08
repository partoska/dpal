#!/usr/bin/env node

/*
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

"use strict";

const { spawn } = require("child_process");

// Map `${process.platform}-${process.arch}` to the package + relative binary.
const TARGETS = {
  "darwin-x64": ["@partoska/dpal-darwin", "bin/dpal"],
  "darwin-arm64": ["@partoska/dpal-darwin", "bin/dpal"],
  "linux-x64": ["@partoska/dpal-linux-x64", "bin/dpal"],
  "linux-arm64": ["@partoska/dpal-linux-arm64", "bin/dpal"],
  "linux-arm": ["@partoska/dpal-linux-arm", "bin/dpal"],
};

const key = `${process.platform}-${process.arch}`;
const target = TARGETS[key];
if (!target) {
  console.error(
    `dpal: unsupported platform/architecture "${key}".\n` +
      `Supported: ${Object.keys(TARGETS).join(", ")}.`,
  );
  process.exit(1);
}

const [pkg, relPath] = target;
let binary;
try {
  binary = require.resolve(`${pkg}/${relPath}`);
} catch {
  console.error(
    `dpal: could not find the native binary for "${key}".\n` +
      `The optional dependency "${pkg}" does not seem to be installed.\n` +
      `Try reinstalling dpal (e.g. "npm install -g @partoska/dpal"), and make\n` +
      `sure optional dependencies are not disabled.`,
  );
  process.exit(1);
}

const args = process.argv.slice(2);

// Prefer replacing this Node process with the native binary (Node >= 22.15).
// The control process (`dpal -C`) is long-lived, so it should own the PID and
// receive signals directly rather than sit behind a Node wrapper.
if (typeof process.execve === "function") {
  try {
    process.execve(binary, [binary, ...args], process.env);
  } catch (err) {
    // Fall through to the spawn-based launcher below.
  }
}

const child = spawn(binary, args, { stdio: "inherit" });

// Forward termination signals so the wrapper behaves like the binary itself.
for (const sig of ["SIGINT", "SIGTERM", "SIGHUP", "SIGQUIT", "SIGUSR1", "SIGUSR2"]) {
  process.on(sig, () => {
    if (child.exitCode === null && child.signalCode === null) {
      child.kill(sig);
    }
  });
}

child.on("error", (err) => {
  console.error(`dpal: failed to launch native binary: ${err.message}`);
  process.exit(1);
});

// Mirror the child's exit.
child.on("exit", (code, signal) => {
  if (signal) {
    process.removeAllListeners(signal);
    process.kill(process.pid, signal);
  } else {
    process.exit(code === null ? 1 : code);
  }
});
