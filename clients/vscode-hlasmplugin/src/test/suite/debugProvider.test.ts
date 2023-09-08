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

import * as assert from 'assert';
import * as vscode from 'vscode';
import * as path from 'path';
import { HLASMConfigurationProvider, getCurrentProgramName } from '../../debugProvider';
import * as helper from './testHelper';

suite('Debug Provider Test Suite', () => {

    suiteTeardown(async function () {
        await helper.closeAllEditors();
    });

    test('Debug Configuration Provider test', async () => {
        const debugProvider = new HLASMConfigurationProvider();

        // resolve empty configuration
        let result = await Promise.resolve(debugProvider.resolveDebugConfiguration(vscode.workspace.workspaceFolders![0], {
            name: '',
            request: '',
            type: ''
        }));
        assert.ok(result);
        assert.strictEqual(result.type, 'hlasm');
        assert.strictEqual(result.name, 'Macro tracer: current program');

        // resolve defined configuration
        result = await Promise.resolve(debugProvider.resolveDebugConfiguration(vscode.workspace.workspaceFolders![0], {
            name: 'Macro tracer: Ask for file name',
            request: 'launch',
            type: 'hlasm'
        }));
        assert.ok(result);
        assert.strictEqual(result.type, 'hlasm');
        assert.strictEqual(result.name, 'Macro tracer: Ask for file name');
    });

    test('Get current program name test', async () => {
        // no editor
        assert.strictEqual(getCurrentProgramName(), undefined);

        // non HLASM file active
        await helper.showDocument('plain.txt');
        assert.strictEqual(getCurrentProgramName(), undefined);

        // HLASM file active
        await helper.showDocument('test', 'hlasm');
        assert.strictEqual(getCurrentProgramName(), path.join(vscode.workspace.workspaceFolders![0].uri.fsPath, 'test'));
    }).slow(1000);
});
