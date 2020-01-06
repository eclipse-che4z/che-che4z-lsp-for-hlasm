#!/usr/bin/env node
/*
 * Copyright (c) 2019 Broadcom.
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


let path  = require('path');
let shell = require('shelljs');

//clean built files
let root = path.dirname(path.dirname(__dirname));
shell.rm('-rf',path.join(root,'node_modules'));
console.log("node_modules removed");
shell.rm('-rf',path.join(root,'lib'));
console.log("lib files removed");
shell.rm('-rf',path.join(root,'language_server.exe'));
shell.rm('-rf',path.join(root,'parser_library.dll'));
shell.rm('-rf',path.join(root,'antlr4-runtime.dll'));
console.log("built files removed");