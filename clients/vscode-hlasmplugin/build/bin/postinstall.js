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

// remove non-standard LIST command
const basic_ftp_client_js = path.join(base, "node_modules", "basic-ftp", "dist", "Client.js");
const basic_ftp_client = fs.readFileSync(basic_ftp_client_js, "utf8");
fs.writeFileSync(basic_ftp_client_js, basic_ftp_client.replace('const LIST_COMMANDS_DEFAULT = ["LIST -a", "LIST"];', 'const LIST_COMMANDS_DEFAULT = ["LIST"];'));
// some servers hard close the data channel when no traffic occured, causing ECONNRESET to be raised, tolerate such behavior
const basic_ftp_ftpcontext_js = path.join(base, "node_modules", "basic-ftp", "dist", "FtpContext.js");
const basic_ftp_ftpcontext = fs.readFileSync(basic_ftp_ftpcontext_js, "utf8");
fs.writeFileSync(basic_ftp_ftpcontext_js, basic_ftp_ftpcontext.replace('socket.once("error", error => {', `socket.once("error", error => {
            if(socket.bytesRead === 0 && error.code === 'ECONNRESET') return;`));
