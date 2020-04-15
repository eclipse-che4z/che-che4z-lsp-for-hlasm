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

import * as path from 'path';
import { runTests } from 'vscode-test';
import { TestOptions } from 'vscode-test/out/runTest';
import * as fs from 'fs';

async function main() {
	try {
		// copy test workspace from src to lib
		const destWorkspacePath = path.join(__dirname, './workspace/');
		const originWorkspacePath = path.join(__dirname, '../../src/test/workspace/');
		recursiveRemoveSync(destWorkspacePath);
		recursiveCopySync(originWorkspacePath,destWorkspacePath);

		// prepare development and tests paths
		const extensionDevelopmentPath = path.join(__dirname, '../../');
		const extensionTestsPath = path.join(__dirname, './suite/index');
		const launchArgs = [destWorkspacePath];
		const options: TestOptions = {
			extensionDevelopmentPath,
			extensionTestsPath,
			launchArgs
		}
		// run tests
		await runTests(options);
	} catch (error) {
		console.error('Tests Failed');
		recursiveReadDirSync(path.join(__dirname,'../../.vscode-test/vscode-1.44.1/VSCode-linux-x64'));
		process.exit(1);
	}
}

function recursiveReadDirSync(dest: string) {
	if (fs.existsSync(dest)) {
		if (fs.statSync(dest).isDirectory()) {
			fs.readdirSync(dest).forEach(file => {
				if (file == 'code') {
					console.log(fs.statSync(path.join(dest,file)).mode);
					return;
				}
				recursiveReadDirSync(path.join(dest,file));
			})
		}
	} else {
		console.log('no dest');
	}
}

function recursiveCopySync(origin: string, dest: string) {
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
};

function recursiveRemoveSync(dest: string) {
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

main();