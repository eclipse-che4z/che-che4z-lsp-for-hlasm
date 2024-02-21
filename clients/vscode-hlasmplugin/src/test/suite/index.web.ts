/*
 * Copyright (c) 2023 Broadcom.
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

import { default as Mocha } from 'mocha/mocha';
import * as vscode from 'vscode';
import { popWaitRequestResolver } from './testHelper';
import { EXTENSION_ID, activate } from '../../extension';

async function registerTestImplementations(): Promise<vscode.Disposable[]> {
	const ext = await vscode.extensions.getExtension<ReturnType<typeof activate>>(EXTENSION_ID)!.activate();

	ext.registerExternalFileClient('TEST', {
		async parseArgs(p: string, _purpose) {
			const [path, file] = p.split('/').slice(1).map(decodeURIComponent).map(x => x.toUpperCase());
			return {
				details: {
					path: path || '',
					file: (file || '').split('.')[0],
					toDisplayString() { return `${path}/${file}`; },
					normalizedPath() { return `/${path}/${file}`; },
				},
				server: undefined,
			}
		},

		listMembers: (arg) => {
			const { path } = arg;
			return Promise.resolve(['MACA', 'MACB', 'MACC'].map(x => `/${path}/${x}`));
		},

		readMember: (args) => {
			if (/^MAC[A-C]$/.test(args.file))
				return Promise.resolve(`.*
          MACRO
          ${args.file}
          MEND`);

			return Promise.resolve(null);
		},
	});

	ext.registerExternalConfigurationProvider((uri: vscode.Uri) => {
		const uriString = uri.toString();
		if (uriString.includes("AAAAA"))
			return {
				configuration: {
					name: "P1",
					asm_options: {
						SYSPARM: "AAAAA"
					},
					libs: [
						{
							path: "libs"
						},
						"copy"
					]
				}
			};
		else if (uriString.includes("BBBBB"))
			return {
				configuration: 'P1'
			};
		else
			return null;
	});

	return [vscode.debug.registerDebugAdapterTrackerFactory('hlasm', {
		createDebugAdapterTracker: function(session: vscode.DebugSession): vscode.ProviderResult<vscode.DebugAdapterTracker> {
			return {
				onDidSendMessage: (message: any) => {
					if (message.type !== 'response')
						return;
					const resolver = popWaitRequestResolver(message.command, session.id);
					if (resolver)
						resolver();
				}
			};
		}
	})];
}

export async function run(): Promise<void> {
	const mocha = Mocha.setup({ ui: 'tdd', color: false, reporter: null });

	await import('./asyncMutex.test.js');
	await import('./codeActions.test.js');
	await import('./commentEditorCommands.test.js');
	await import('./completionList.test.js');
	await import('./configurationsHandler.test.js');
	await import('./connectionPool.test.js');
	await import('./continuationHandler.test.js');
	await import('./conversions.test.js');
	await import('./customEditorCommands.test.js');
	await import('./debugProvider.test.js');
	await import('./debugging.test.js');
	await import('./eventsHandler.test.js');
	//await import('./fbUtils.test.js');
	//await import('./ftpUtils.test.js');
	//await import('./hlasmDownloadCommands.test.js');
	await import('./hlasmExternalConfigurationProvider.test.js');
	await import('./hlasmExternalFiles.test.js');
	await import('./hlasmExternalFilesEndevor.test.js');
	//await import('./hlasmExternalFilesFtp.test.js');
	await import('./hlasmLanguageDetection.test.js');
	await import('./hlasmListingServices.test.js');
	await import('./integration.test.js');
	await import('./utils.test.js');

	const toDispose = await registerTestImplementations();

	await new Promise((resolve, reject) => {
		// Run the mocha test
		mocha.run(failures => {
			if (failures > 0) {
				reject(new Error(`${failures} tests failed.`));
			} else {
				resolve(undefined);
			}
		});
	}).finally(() => { toDispose.forEach(d => d.dispose()) });
}
