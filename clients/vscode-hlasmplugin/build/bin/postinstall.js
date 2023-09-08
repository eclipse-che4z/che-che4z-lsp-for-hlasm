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

// vscode-test-web patch
const vscode_test_web_js = path.join(base, "node_modules", "@vscode", "test-web", "fs-provider", "dist", "fsExtensionMain.js");
const vscode_test_web = fs.readFileSync(vscode_test_web_js, "utf8");
fs.writeFileSync(vscode_test_web_js, vscode_test_web
    .replace("const url = serverUri.with({ query: 'readdir' }).toString(/*skipEncoding*/ true);", "const url = serverUri.with({ query: 'readdir' }).toString(/*skipEncoding*/ false);")
    .replace("const url = serverUri.with({ query: 'stat' }).toString(/*skipEncoding*/ true);", "const url = serverUri.with({ query: 'stat' }).toString(/*skipEncoding*/ false);")
    .replace("const response = await (0, request_light_1.xhr)({ url: serverUri.toString(/*skipEncoding*/ true) });", "const response = await (0, request_light_1.xhr)({ url: serverUri.toString(/*skipEncoding*/ false) });")
);

const vscode_test_web_mounts_js = path.join(base, "node_modules", "@vscode", "test-web", "out", "server", "mounts.js");
const vscode_test_web_mounts = fs.readFileSync(vscode_test_web_mounts_js, "utf8");
fs.writeFileSync(vscode_test_web_mounts_js, vscode_test_web_mounts
    .replaceAll("const p = path.join(folderMountPath, ctx.path.substring(mountPrefix.length));", "const p = path.join(folderMountPath, decodeURIComponent(ctx.path.substring(mountPrefix.length)));")
);
