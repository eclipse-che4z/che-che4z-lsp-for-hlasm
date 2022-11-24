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
import * as Mocha from 'mocha';
import * as glob from 'glob';
import * as vscode from 'vscode';
import * as process from 'process';

export async function run(): Promise<void> {
	const is_vscode = process.execPath.includes('Code');

	// Create the mocha test
	const mocha = new Mocha({ ui: 'tdd', color: true });
	const testsPath = path.join(__dirname, '..');

	const files = await new Promise<string[]>((resolve, reject) => {
		glob((is_vscode) ? '**/**.test.js' : '**/integration.test.js', { cwd: testsPath }, (err, files) => {
			if (err)
				reject(err);
			else
				resolve(files);
		});
	});

	// Add files to the test suite
	files.forEach(file =>
		mocha.addFile(path.resolve(testsPath, file)));

	await new Promise((resolve, reject) => {
		// Run the mocha test
		mocha.run(failures => {
			if (failures > 0) {
				if (!is_vscode)
					console.error('>>>THEIA TESTS FAILED<<<');
				reject(new Error(`${failures} tests failed.`));
			} else {
				resolve(undefined);
			}
		});
	});

	if (!is_vscode)
		console.log('>>>THEIA TESTS PASSED<<<');
}
