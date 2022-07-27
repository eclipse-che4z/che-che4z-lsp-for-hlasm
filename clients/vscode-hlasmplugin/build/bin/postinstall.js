#!/usr/bin/env node
/*
 * Copyright (c) 2022 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

const fs = require('fs');
const path = require('path');

const base = path.join(__dirname, "..", "..");

fs.mkdirSync(path.join(base, "bin"), { recursive: true });
fs.copyFileSync(path.join(base, "terse", "terse.js"), path.join(base, "bin", "terse.js"));
fs.copyFileSync(path.join(base, "terse", "terse.wasm"), path.join(base, "bin", "terse.wasm"));
