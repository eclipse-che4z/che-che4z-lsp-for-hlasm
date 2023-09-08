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

const fs = require('fs')
const path = require('path')

const workspace_dir = path.join(__dirname, '..', '..', 'dist_test/workspace/');
const workspace_vscode = path.join(workspace_dir, '.vscode');
recursiveRemoveSync(workspace_dir);
recursiveCopySync(path.join(__dirname, '..', '..', 'src/test/workspace/'), workspace_dir);

const source_settings_file = (function () {
    if (process.argv.indexOf('wasm') !== -1) {
        console.log('Preparing WASM');
        return 'settings.wasm.json';
    }
    else {
        console.log('Preparing native');
        return 'settings.native.json';
    }
})();

recursiveCopySync(path.join(workspace_vscode, source_settings_file), path.join(workspace_vscode, 'settings.json'));

console.log('Test workspace ready')

function recursiveCopySync(origin, dest) {
    if (fs.existsSync(origin)) {
        if (fs.statSync(origin).isDirectory()) {
            fs.mkdirSync(dest, { recursive: true });
            fs.readdirSync(origin).forEach(file =>
                recursiveCopySync(path.join(origin, file), path.join(dest, file)));
        }
        else {
            fs.copyFileSync(origin, dest);
        }
    }
}

function recursiveRemoveSync(dest) {
    if (fs.existsSync(dest)) {
        fs.readdirSync(dest).forEach(file => {
            const currPath = path.join(dest, file);
            if (fs.statSync(currPath).isDirectory()) {
                recursiveRemoveSync(currPath);
            }
            else {
                fs.unlinkSync(currPath);
            }
        });
        fs.rmdirSync(dest);
    }
}
