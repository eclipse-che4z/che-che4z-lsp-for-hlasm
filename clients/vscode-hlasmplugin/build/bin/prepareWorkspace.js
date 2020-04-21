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

recursiveRemoveSync(path.join(__dirname,'..','..','lib/test/workspace/'));
recursiveCopySync(path.join(__dirname,'..','..','src/test/workspace/'),path.join(__dirname,'..','..','lib/test/workspace/'));

function recursiveCopySync(origin, dest) {
    if (fs.existsSync(origin)) {
        if (fs.statSync(origin).isDirectory()) {
            fs.mkdirSync(dest);
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
