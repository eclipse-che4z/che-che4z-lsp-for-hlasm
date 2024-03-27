/*
 * Copyright (c) 2024 Broadcom.
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

import * as vscode from 'vscode';
import * as assert from 'assert';
import { showDocument } from './testHelper';

suite('Output content provider', () => {

    test('Show outputs', async () => {
        const _ = await showDocument('output.hlasm', 'hlasm');

        const editorPromise = new Promise<vscode.TextEditor>((res, _) => {
            const disp = vscode.window.onDidChangeVisibleTextEditors(editors => {
                const editor = editors.find(x => x.document.uri.scheme === 'hlasm-output');
                if (editor) {
                    disp.dispose();
                    res(editor);
                }
            });
        });

        await vscode.commands.executeCommand('extension.hlasm-plugin.showOutput', { mnote: true, punch: true });

        const editor = await editorPromise;

        assert.ok(editor);

        assert.strictEqual(editor.document.getText(), 'punch\n002:mnote');

        await vscode.commands.executeCommand('workbench.action.closeActiveEditor', editor);
    }).timeout(10000).slow(2000);

});
