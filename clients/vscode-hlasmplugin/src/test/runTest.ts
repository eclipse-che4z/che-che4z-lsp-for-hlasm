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
import { runTests, downloadAndUnzipVSCode } from '@vscode/test-electron';
import * as process from 'process';
import * as os from 'os'
import * as fs from 'fs'

async function main() {
	try {
		// prepare development and tests paths
		const extensionDevelopmentPath = path.join(__dirname, '../../../');
		const extensionTestsPath = path.join(__dirname, './suite/index');
		const launchArgs = [
			path.join(__dirname, '../workspace/'),
			'--disable-extensions',
			'--disable-workspace-trust',
			'--user-data-dir',
			fs.mkdtempSync(path.join(os.tmpdir(), 'test-'))
		];
		const vscodeExecutablePath = process.argv.length > 2 && process.argv[2] == 'insiders' && await downloadAndUnzipVSCode('insiders') || undefined;

		// run tests
		await runTests({
			vscodeExecutablePath,
			extensionDevelopmentPath,
			extensionTestsPath,
			launchArgs
		});
	} catch (error) {
		console.log(error);
		console.error('Tests Failed');
		process.exit(1);
	}
}

main();
