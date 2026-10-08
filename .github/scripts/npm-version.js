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

// Stamp or verify the version of every npm package under npm/.
//
// Usage: node .github/scripts/npm-version.js stamp|verify <version>
//
// The main package (npm/dpal) also pins each platform package in its
// optionalDependencies, which must match the same version.

"use strict";

const fs = require("fs");
const path = require("path");

const ROOT = path.join(__dirname, "..", "..", "npm");
const MAIN = "dpal";

const [mode, version] = process.argv.slice(2);
if (!["stamp", "verify"].includes(mode) || !/^\d+\.\d+\.\d+$/.test(version || "")) {
  console.error("usage: npm-version.js stamp|verify <x.y.z>");
  process.exit(2);
}

const dirs = fs
  .readdirSync(ROOT)
  .filter((d) => fs.existsSync(path.join(ROOT, d, "package.json")));
const platforms = dirs.filter((d) => d !== MAIN);

let ok = true;
for (const dir of dirs) {
  const file = path.join(ROOT, dir, "package.json");
  const pkg = JSON.parse(fs.readFileSync(file, "utf8"));

  if (dir === MAIN) {
    const expected = platforms.map((p) => `@partoska/${p}`).sort();
    const actual = Object.keys(pkg.optionalDependencies || {}).sort();
    if (JSON.stringify(expected) !== JSON.stringify(actual)) {
      console.error(
        `::error::npm/${MAIN} optionalDependencies [${actual}] do not match platform packages [${expected}]`,
      );
      ok = false;
    }
  }

  if (mode === "stamp") {
    pkg.version = version;
    for (const k of Object.keys(pkg.optionalDependencies || {})) {
      pkg.optionalDependencies[k] = version;
    }
    fs.writeFileSync(file, JSON.stringify(pkg, null, 2) + "\n");
    continue;
  }

  if (pkg.version !== version) {
    console.error(`::error::npm/${dir}/package.json version ${pkg.version} != ${version}`);
    ok = false;
  }
  for (const [k, v] of Object.entries(pkg.optionalDependencies || {})) {
    if (v !== version) {
      console.error(`::error::npm/${dir} optionalDependency ${k} pinned to ${v} != ${version}`);
      ok = false;
    }
  }
}

if (!ok) process.exit(1);
console.log(`${mode === "stamp" ? "stamped" : "verified"} ${dirs.length} packages at ${version}`);
