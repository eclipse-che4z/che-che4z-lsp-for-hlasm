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

const rate_limit_js = path.join(base, "node_modules", "@semantic-release", "github", "lib", "definitions", "rate-limit.js");
const rateLimits = fs.readFileSync(rate_limit_js, "utf8")
fs.writeFileSync(rate_limit_js, rateLimits.replace("minTimeout: 1000", "minTimeout: 5000").replace("GLOBAL_RATE_LIMIT = 1000", "GLOBAL_RATE_LIMIT = 5000").replaceAll("* 1.1", "* 3"));

const get_client_js = path.join(base, "node_modules", "@semantic-release", "github", "lib", "get-client.js");
const getClient = fs.readFileSync(get_client_js, "utf8");
fs.writeFileSync(get_client_js, getClient.replace("catch (error) {", `catch (error) {
        if (error.status == 403 && /exceeded a secondary rate limit/i.test(error.message)) {
          const resetTimestamp = +(error.response && error.response.headers && error.response.headers['x-ratelimit-reset'] || '') * 1000;
          await new Promise((resolve) => setTimeout(resolve, Math.max(5000, resetTimestamp - Date.now())));
          throw error;
        }`));

// remove non-standard LIST command
const basic_ftp_client_js = path.join(base, "node_modules", "basic-ftp", "dist", "Client.js");
const basic_ftp_client = fs.readFileSync(basic_ftp_client_js, "utf8");
fs.writeFileSync(basic_ftp_client_js, basic_ftp_client.replace('const LIST_COMMANDS_DEFAULT = ["LIST -a", "LIST"];', 'const LIST_COMMANDS_DEFAULT = ["LIST"];'));
// some servers hard close the data channel when no traffic occured, causing ECONNRESET to be raised, tolerate such behavior
const basic_ftp_ftpcontext_js = path.join(base, "node_modules", "basic-ftp", "dist", "FtpContext.js");
const basic_ftp_ftpcontext = fs.readFileSync(basic_ftp_ftpcontext_js, "utf8");
fs.writeFileSync(basic_ftp_ftpcontext_js, basic_ftp_ftpcontext.replace('socket.once("error", error => {', `socket.once("error", error => {
            if(socket.bytesRead === 0 && error.code === 'ECONNRESET') return;`));
