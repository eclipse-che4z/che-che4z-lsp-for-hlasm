/*
 * Copyright (c) 2022 Broadcom.
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
import * as helper from './testHelper';

suite('Debugging Test Suite', () => {
    let editor: vscode.TextEditor;
    const workspace_file = 'open';

    suiteSetup(async function () {
        this.timeout(10000);

        editor = (await helper.showDocument(workspace_file)).editor;
    });

    suiteTeardown(async function () {
        this.timeout(10000);

        await helper.removeAllBreakpoints();
        await helper.closeAllEditors();
    });

    // debug open code test
    test('Debug test', async () => {
        const session = await helper.debugStartSession();

        // Start by stepping into a macro and checking the file has been accessed
        await helper.debugStepOver(4);
        await helper.debugStepInto();
        assert.strictEqual(vscode.window.activeTextEditor.document.uri.fsPath, path.join(helper.getWorkspacePath(), 'libs', 'mac.asm'), 'Wrong macro file entered');

        // Step out and check the file
        await helper.debugStepOver(3);
        assert.strictEqual(vscode.window.activeTextEditor.document.uri.fsPath, editor.document.uri.fsPath, 'Stepped out to a wrong file');

        await helper.debugStop();

    }).timeout(20000).slow(10000);

    test('Breakpoint test', async () => {
        await helper.addBreakpoints(path.join('libs', 'mac.asm'), [3]);
        await helper.addBreakpoints('open', [3, 9]);

        await helper.debugStartSession();

        // Continue until breakpoint is hit
        await helper.debugContinue();
        assert.strictEqual(vscode.window.activeTextEditor.document.uri.fsPath, editor.document.uri.fsPath, 'Expected to be in the source file');

        // Continue until breakpoint is hit
        await helper.debugContinue();
        assert.strictEqual(vscode.window.activeTextEditor.document.uri.fsPath ,  path.join(helper.getWorkspacePath(), 'libs', 'mac.asm'), 'Expected to be in the macro file');

        // Continue until breakpoint is hit
        await helper.debugContinue();
        assert.strictEqual(vscode.window.activeTextEditor.document.uri.fsPath, editor.document.uri.fsPath, 'Expected to be in the source file');

        await helper.debugStop();

    }).timeout(20000).slow(10000);

    // verify that virtual files are working
    test('Virtual files', async () => {
        await helper.addBreakpoints('virtual', [7, 11, 12]);

        await helper.debugStartSession();

        // Continue until breakpoint is hit
        await helper.debugContinue();
        assert.strictEqual(vscode.window.activeTextEditor.document.uri.fsPath, path.join(helper.getWorkspacePath(), 'virtual'), 'Expected to be in the source file');

        // Step into a virtual file
        await helper.debugStepInto();
        assert.strictEqual(vscode.window.activeTextEditor.document.uri.path, '/AINSERT_1.hlasm', 'Expected to be in the virtual AINSERT file');

        // Continue until breakpoint is hit and enter the virtual file again
        await helper.debugContinue();
        await helper.debugStepInto();
        assert.strictEqual(vscode.window.activeTextEditor.document.uri.path, '/AINSERT_1.hlasm', 'Expected to be in the virtual AINSERT file');

        // Continue until breakpoint is hit and enter virtual file through generated macro
        await helper.debugContinue();
        assert.strictEqual(vscode.window.activeTextEditor.document.uri.fsPath, path.join(helper.getWorkspacePath(), 'virtual'), 'Expected to be in the source file');

        await helper.debugStop();
    }).timeout(20000).slow(10000);
});
