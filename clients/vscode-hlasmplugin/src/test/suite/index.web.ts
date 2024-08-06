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
import { activate } from '../../extension';
import { EXTENSION_ID, debugTypeHlasm } from '../../constants';

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

    return [vscode.debug.registerDebugAdapterTrackerFactory(debugTypeHlasm, {
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
    const toDispose = await registerTestImplementations();

    const mocha = Mocha.setup({ ui: 'tdd', color: false, reporter: null });

    await Promise.all([
        import('./asyncMutex.test.js'),
        import('./codeActions.test.js'),
        import('./commentEditorCommands.test.js'),
        import('./completionList.test.js'),
        import('./configurationsHandler.test.js'),
        import('./connectionPool.test.js'),
        import('./continuationHandler.test.js'),
        import('./conversions.test.js'),
        import('./customEditorCommands.test.js'),
        import('./debugProvider.test.js'),
        import('./debugging.test.js'),
        import('./eventsHandler.test.js'),
        // import('./fbUtils.test.js'),
        // import('./ftpUtils.test.js'),
        // import('./hlasmDownloadCommands.test.js'),
        import('./hlasmExternalConfigurationProvider.test.js'),
        import('./hlasmExternalFiles.test.js'),
        import('./hlasmExternalFilesEndevor.test.js'),
        // import('./hlasmExternalFilesFtp.test.js'),
        import('./hlasmLanguageDetection.test.js'),
        import('./hlasmListingServices.test.js'),
        import('./hlasmOutputContentProvider.test.js'),
        import('./integration.test.js'),
        import('./utils.test.js'),
    ]);

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
