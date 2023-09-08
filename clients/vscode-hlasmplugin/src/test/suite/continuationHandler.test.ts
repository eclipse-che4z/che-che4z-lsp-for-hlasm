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

import { insertContinuation, rearrangeSequenceNumbers, removeContinuation } from '../../continuationHandler';
import { TextDocumentMock, TextEditorEditMock, TextEditorMock } from '../mocks';

suite('Continuation Handler Test Suite', () => {
    const config = vscode.workspace.getConfiguration('hlasm');

    test('Insert Continuation Test', () => {
        assert.ok(config.get('continuationHandling'));
        // prepare document
        const document = new TextDocumentMock();
        document.uri = vscode.Uri.file('file');
        // prepare editor and edit
        const editor = new TextEditorMock(document);
        const cursorPosition = new vscode.Position(0, 4);
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        const edit = new TextEditorEditMock('this');
        document.text = edit.text;

        // insert new continuation
        insertContinuation(editor, edit, 15, 5);
        document.text = edit.text;
        assert.strictEqual(document.text, 'this           X\r\n     ');

        // insert continuation on continued line
        insertContinuation(editor, edit, 15, 5);
        document.text = edit.text;
        assert.strictEqual(document.text, 'this           X\r\n               X\r\n     ');
    });

    test('Insert Continuation Test - Multiple cursors', () => {
        assert.ok(config.get('continuationHandling'));
        // prepare document
        const document = new TextDocumentMock();
        document.uri = vscode.Uri.file('file');
        // prepare editor and edit
        const editor = new TextEditorMock(document);
        editor.selections = [
            new vscode.Selection(new vscode.Position(0, 17), new vscode.Position(0, 22)),
            new vscode.Selection(new vscode.Position(0, 27), new vscode.Position(0, 32)),
        ];
        const edit = new TextEditorEditMock('label instr arg1,arg2,arg3,arg4   comment');
        document.text = edit.text;

        // insert new continuation
        insertContinuation(editor, edit, 50, 5);
        document.text = edit.text;
        assert.strictEqual(document.text, 'label instr arg1,arg3,  comment                   X\r\n     arg2,arg4');
    });

    test('Insert Continuation Test - detect continuation', () => {
        assert.ok(config.get('continuationHandling'));
        // prepare document
        const document = new TextDocumentMock();
        document.uri = vscode.Uri.file('file');
        // prepare editor and edit
        const editor = new TextEditorMock(document);
        editor.selections = [
            new vscode.Selection(new vscode.Position(2, 7), new vscode.Position(2, 7)),
        ];
        const edit = new TextEditorEditMock('    aaa   +\r\n\r\n    bbb');
        document.text = edit.text;

        // insert new continuation
        insertContinuation(editor, edit, 10, 5);
        document.text = edit.text;
        assert.strictEqual(document.text, '    aaa   +\r\n\r\n    bbb   +\r\n     ');
    });

    test('Remove Continuation Test', () => {
        assert.ok(config.get('continuationHandling'));
        // prepare document
        const document = new TextDocumentMock();
        document.uri = vscode.Uri.file('file');
        // prepare editor and edit
        const editor = new TextEditorMock(document);
        let cursorPosition = new vscode.Position(1, 0);
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        const edit = new TextEditorEditMock('continuation   X\r\n     ');
        document.text = edit.text;

        // delete existing continuation
        removeContinuation(editor, edit, 15);
        document.text = edit.text;
        assert.strictEqual(document.text, 'continuation    ');

        // prepare document
        cursorPosition = new vscode.Position(0, 0);
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);

        // delete non existing continuation - nothing happens
        removeContinuation(editor, edit, 15);
        document.text = edit.text;
        assert.strictEqual(document.text, 'continuation    ');
    });

    test('Rearrange sequence numbers', () => {
        assert.ok(config.get('continuationHandling'));
        // prepare document
        const document = new TextDocumentMock();
        document.uri = vscode.Uri.file('file');
        // prepare editor and edit
        const editor = new TextEditorMock(document);
        editor.selections = [
            new vscode.Selection(new vscode.Position(0, 0), new vscode.Position(0, 0))
        ];
        const edit = new TextEditorEditMock('label instr arg1,arg2,arg3,arg4   comment           12345678');
        document.text = edit.text;

        // insert new continuation
        rearrangeSequenceNumbers(editor, edit, 50);
        document.text = edit.text;
        assert.strictEqual(document.text, 'label instr arg1,arg2,arg3,arg4   comment          12345678');
    });

    test('Rearrange sequence numbers - deletion', () => {
        assert.ok(config.get('continuationHandling'));
        // prepare document
        const document = new TextDocumentMock();
        document.uri = vscode.Uri.file('file');
        // prepare editor and edit
        const editor = new TextEditorMock(document);
        editor.selections = [
            new vscode.Selection(new vscode.Position(0, 26), new vscode.Position(0, 31)),
        ];
        const edit = new TextEditorEditMock('label instr arg1,arg2,arg3,arg4   comment          12345678');
        document.text = edit.text;

        // insert new continuation
        rearrangeSequenceNumbers(editor, edit, 50);
        document.text = edit.text;
        assert.strictEqual(document.text, 'label instr arg1,arg2,arg3   comment               12345678');
    });

});
