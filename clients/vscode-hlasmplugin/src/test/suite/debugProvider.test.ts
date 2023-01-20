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
import { DebugConfigurationMock } from '../mocks';
import * as helper from './testHelper';

suite('Debug Provider Test Suite', () => {

    suiteTeardown(async function () {
        await helper.closeAllEditors();
    });

    test('Debug Configuration Provider test', () => {
        const debugProvider = new HLASMConfigurationProvider();

        // resolve empty configuration
        const emptyDebugConf = new DebugConfigurationMock();
        let result = <vscode.DebugConfiguration>(debugProvider.resolveDebugConfiguration(vscode.workspace.workspaceFolders[0], emptyDebugConf));
        assert.equal(result.type, 'hlasm');
        assert.equal(result.name, 'Macro tracer: current program');

        // resolve defined configuration
        const debugConf = new DebugConfigurationMock();
        debugConf.name = 'Macro tracer: Ask for file name';
        debugConf.request = 'launch';
        debugConf.type = 'hlasm';
        result = <vscode.DebugConfiguration>(debugProvider.resolveDebugConfiguration(vscode.workspace.workspaceFolders[0], debugConf));
        assert.equal(result.type, 'hlasm');
        assert.equal(result.name, 'Macro tracer: Ask for file name');
    });

    test('Get current program name test', async () => {
        // no editor
        assert.equal(getCurrentProgramName(), undefined);

        // non HLASM file active
        await helper.showDocument('plain.txt');
        assert.equal(getCurrentProgramName(), undefined);

        // HLASM file active
        await helper.showDocument('test', 'hlasm');
        assert.equal(getCurrentProgramName(), path.join(vscode.workspace.workspaceFolders[0].uri.fsPath, 'test'));
    }).slow(1000);
});
