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

suite('Code actions', () => {
    suiteSetup(async function () {
        this.timeout(10000);
    });

    suiteTeardown(async function () {
        await helper.closeAllEditors();
    });

    test('Diagnostics not suppressed', async () => {
        const diagnostic_event = helper.waitForDiagnostics();

        const { editor, document } = await helper.showDocument('code_action_1.hlasm', 'hlasm');

        await diagnostic_event;

        const codeActionsList: vscode.CodeAction[] = await vscode.commands.executeCommand('vscode.executeCodeActionProvider', document.uri, new vscode.Range(0, 10, 0, 15));

        assert.equal(codeActionsList.length, 4 + 3);
    }).timeout(10000).slow(5000);

    test('Diagnostics suppressed', async () => {
        const diagnostic_event = helper.waitForDiagnostics();

        const { editor, document } = await helper.showDocument('code_action_2.hlasm', 'hlasm');

        await diagnostic_event;

        const codeActionsList: vscode.CodeAction[] = await vscode.commands.executeCommand('vscode.executeCodeActionProvider', document.uri, new vscode.Range(0, 10, 0, 15));

        assert.equal(codeActionsList.length, 1);
    }).timeout(10000).slow(5000);
});
