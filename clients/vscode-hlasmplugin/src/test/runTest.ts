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

async function main() {
	try {
		// prepare development and tests paths
		const extensionDevelopmentPath = path.join(__dirname, '../../');
		const extensionTestsPath = path.join(__dirname, './suite/index');
		const launchArgs = [path.join(__dirname, './workspace/')];
		const options: TestOptions = {
			extensionDevelopmentPath,
			extensionTestsPath,
			launchArgs
		}
		// run tests
		await runTests(options);
	} catch (error) {
		console.error('Tests Failed');
		process.exit(1);
	}
}

main();
