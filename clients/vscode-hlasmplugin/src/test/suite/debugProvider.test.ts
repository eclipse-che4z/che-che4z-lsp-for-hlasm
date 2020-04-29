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

suite('Debug Test Suite', () => {

    test('Debug Configuration Provider test', () => {
        const debugProvider = new HLASMConfigurationProvider(0);

        // resolve empty configuration
        const emptyDebugConf = new DebugConfigurationMock();
        var result = <vscode.DebugConfiguration>(debugProvider.resolveDebugConfiguration(vscode.workspace.workspaceFolders[0], emptyDebugConf));
        assert.equal(result.type, 'hlasm');
        assert.equal(result['debugServer'], 0);
        assert.equal(result.name, 'Macro tracer: current program');

        // resolve defined configuration
        const debugConf = new DebugConfigurationMock();
        debugConf.name = 'Macro tracer: Ask for file name';
        debugConf.request = 'launch';
        debugConf.type = 'hlasm';
        result = <vscode.DebugConfiguration>(debugProvider.resolveDebugConfiguration(vscode.workspace.workspaceFolders[0], debugConf));
        assert.equal(result.type, 'hlasm');
        assert.equal(result['debugServer'], 0);
        assert.equal(result.name, 'Macro tracer: Ask for file name');
    });

    test('Get current program name test', async () => {
        // no editor
        assert.equal(getCurrentProgramName(), undefined);

        // non HLASM file active
        const textDocument = await vscode.workspace.openTextDocument(path.join(vscode.workspace.workspaceFolders[0].uri.fsPath, 'plain.txt'));
        await vscode.window.showTextDocument(textDocument);
        assert.equal(getCurrentProgramName(), undefined);

        // HLASM file active
        const hlasmDocument = await vscode.workspace.openTextDocument(path.join(vscode.workspace.workspaceFolders[0].uri.fsPath, 'open'));
        await vscode.languages.setTextDocumentLanguage(hlasmDocument, 'hlasm');
        await vscode.window.showTextDocument(hlasmDocument);
        assert.equal(getCurrentProgramName(), path.join(vscode.workspace.workspaceFolders[0].uri.fsPath, 'open'));
    }).slow(2000);
});
