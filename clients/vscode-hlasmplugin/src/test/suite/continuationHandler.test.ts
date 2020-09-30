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

import { ContinuationHandler } from '../../continuationHandler';
import { TextDocumentMock, TextEditorEditMock, TextEditorMock } from '../mocks';

suite('Continuation Handler Test Suite', () => {
    const handler = new ContinuationHandler();
    const config = vscode.workspace.getConfiguration('hlasm');

    test('Insert Continuation Test', () => {
        assert.ok(config.get('continuationHandling'));
        // prepare document
        const document = new TextDocumentMock();
        document.uri = vscode.Uri.file('file');
        // prepare editor and edit
        const editor = new TextEditorMock(document);
        const cursorPosition = new vscode.Position(0, 0);
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        const edit = new TextEditorEditMock('this');
        document.text = edit.text;

        // insert new continuation
        handler.insertContinuation(editor, edit, 15,5);
        document.text = edit.text;
        assert.equal(document.text, 'this           X\r\n     ');

        // insert continuation on continued line
        handler.insertContinuation(editor, edit, 15,5);
        document.text = edit.text;
        assert.equal(document.text, 'this           X\r\n               X\r\n     ');
    });

    test('Remove Continuation Test', () => {
        assert.ok(config.get('continuationHandling'));
        // prepare document
        const document = new TextDocumentMock();
        document.uri = vscode.Uri.file('file');
        // prepare editor and edit
        const editor = new TextEditorMock(document);
        var cursorPosition = new vscode.Position(1, 0);
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        const edit = new TextEditorEditMock('continuation   X\r\n     ');
        document.text = edit.text;

        // delete existing continuation
        handler.removeContinuation(editor, edit, 15);
        document.text = edit.text;
        assert.equal(document.text, 'continuation   ');

        // prepare document
        cursorPosition = new vscode.Position(0, 0);
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);

        // delete non existing continuation - nothing happens
        handler.removeContinuation(editor, edit, 15);
        document.text = edit.text;
        assert.equal(document.text, 'continuation   ');
    });

});
