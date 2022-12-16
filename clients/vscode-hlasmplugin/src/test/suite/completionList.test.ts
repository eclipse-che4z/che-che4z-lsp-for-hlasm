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
import * as helper from './testHelper';

suite('Completion List Test Suite', () => {
    const workspace_file = 'open';
    let editor: vscode.TextEditor;

    suiteSetup(async function () {
        this.timeout(10000);

        editor = (await helper.showDocument(workspace_file)).editor;
    });

    suiteTeardown(async function () {
        await helper.closeAllEditors();
    });

    // test completion list for instructions
    test('Completion List Instructions test', async () => {
        const diags = helper.waitForDiagnostics(workspace_file);
        const movePosition = await helper.insertString(editor, new vscode.Position(7, 1), 'L');
        await diags;

        const completionList: vscode.CompletionList = await vscode.commands.executeCommand('vscode.executeCompletionItemProvider', editor.document.uri, movePosition);

        const result = completionList.items.filter(complItem => complItem.label.toString().startsWith('L'));
        assert.ok(result.length === 343, 'Wrong number of suggestion result. Expected 343 got ' + result.length);
    }).timeout(10000).slow(1000);

    // test completion list for variable symbols
    test('Completion List Variable symbols test', async () => {
        // add '&' to simulate start of a variable symbol
        const diags = helper.waitForDiagnostics(workspace_file);
        const movePosition = await helper.insertString(editor, new vscode.Position(8, 0), '&');
        await diags;

        const completionList: vscode.CompletionList = await vscode.commands.executeCommand('vscode.executeCompletionItemProvider', editor.document.uri, movePosition);

        assert.ok(completionList.items.length === 2, 'Wrong number of suggestion result. Expected 2 got ' + completionList.items.length);
        assert.ok(completionList.items[0].label.toString() === '&VAR', 'Wrong first completion item. Expected &VAR got ' + completionList.items[0].label.toString());
        assert.ok(completionList.items[1].label.toString() === '&VAR2', 'Wrong second completion item. Expected &VAR2 got ' + completionList.items[1].label.toString());
    }).timeout(10000).slow(1000);
});
