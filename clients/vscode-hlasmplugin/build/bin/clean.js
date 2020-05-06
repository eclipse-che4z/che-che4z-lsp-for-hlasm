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
let fs = require('fs')

//clean built files
let root = path.join(__dirname,'..','..');
recursiveRemoveSync(path.join(root,'node_modules'));
console.log("node_modules removed");
recursiveRemoveSync(path.join(root,'lib'));
console.log("lib files removed");
recursiveRemoveSync(path.join(root,'bin'));
console.log("built files removed");
recursiveRemoveSync(path.join(root,'coverage'));
console.log("coverage files removed");
recursiveRemoveSync(path.join(root,'.nyc_output'));
console.log("nyc temp files removed");

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
};

