/* --------------------------------------------------------------------------------------------
 * Copyright (c) TypeFox. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */

import * as path from 'path';
import * as Mocha from 'mocha';
import * as glob from 'glob';
import * as vscode from 'vscode';

export function run(): Promise<void> {
	// Create the mocha test
	const mocha = new Mocha({ ui: 'tdd', color: true });
	const testsPath = path.join(__dirname, '..');

	return new Promise((resolve, reject) => {
		glob('**/**.test.js', { cwd: testsPath }, (_, files) => {
				// Add files to the test suite
				files.forEach(file => 
					mocha.addFile(path.resolve(testsPath, file)));
	
				try {
					vscode.workspace.getConfiguration('hlasmplugin').update('continuationHandling',true).then(() => {
						// Run the mocha test
						mocha.run(failures => {
							if (failures > 0) {
								reject(new Error(`${failures} tests failed.`));
							} else {
								resolve();
							}
						});
					});
				} catch (error) {
					console.error(error);
					reject(error);
				}
		});
	});
}