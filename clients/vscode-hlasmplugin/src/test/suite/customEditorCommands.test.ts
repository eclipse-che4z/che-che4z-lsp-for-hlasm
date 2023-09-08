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

import { CustomEditorCommands } from '../../customEditorCommands';
import { TextEditorEditMock, TextEditorMock, TextDocumentMock } from '../mocks';

suite('Custom Editor Commands Test Suite', () => {
    const commands = new CustomEditorCommands();

    test('Insert chars test', () => {
        // prepare document and editor
        const document = new TextDocumentMock();
        const editor = new TextEditorMock(document);
        const cursorPosition = new vscode.Position(0, 0);
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        const edit = new TextEditorEditMock('this is some text    X');
        document.text = edit.text;

        // add 'a' at the beginning of text without continuation
        commands.insertChars(editor, edit, { text: 'a' }, -1);
        document.text = edit.text;
        assert.strictEqual(document.text, 'athis is some text    X');

        // replace 'at' for 'bc' without continuation
        const movedCursorPosition = new vscode.Position(0, 2);
        editor.selection = new vscode.Selection(cursorPosition, movedCursorPosition);
        commands.insertChars(editor, edit, { text: 'bc' }, -1);
        document.text = edit.text;
        assert.strictEqual(document.text, 'bchis is some text    X');

        // simulate continuation on column 22
        // add 'd' at the beginning
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        commands.insertChars(editor, edit, { text: 'd' }, 22);
        document.text = edit.text;
        assert.strictEqual(document.text, 'dbchis is some text   X');
    });

    test('Delete Left chars test', () => {
        // prepare document and editor
        const document = new TextDocumentMock();
        const editor = new TextEditorMock(document);
        const cursorPosition = new vscode.Position(0, 2);
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        const edit = new TextEditorEditMock('this is some text    X');
        document.text = edit.text;

        // remove second character 'h' without continuation
        commands.deleteLeft(editor, edit, -1);
        document.text = edit.text;
        assert.strictEqual(document.text, 'tis is some text    X');

        // remove first 2 characters 'ti' without continuation
        const movedCursorPosition = new vscode.Position(0, 0);
        editor.selection = new vscode.Selection(cursorPosition, movedCursorPosition);
        commands.deleteLeft(editor, edit, -1);
        document.text = edit.text;
        assert.strictEqual(document.text, 's is some text    X');

        // remove second character ' ' with continuation on column 18
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        commands.deleteLeft(editor, edit, 18);
        document.text = edit.text;
        assert.strictEqual(document.text, 'sis some text     X');
    });

    test('Delete Right chars test', () => {
        // prepare document and editor
        const document = new TextDocumentMock();
        const editor = new TextEditorMock(document);
        const cursorPosition = new vscode.Position(0, 0);
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        const edit = new TextEditorEditMock('this is some text    X');
        document.text = edit.text;

        // remove first character 't' without continuation
        commands.deleteRight(editor, edit, -1);
        document.text = edit.text;
        assert.strictEqual(document.text, 'his is some text    X');

        // remove first 2 characters 'hi' without continuation
        const movedCursorPosition = new vscode.Position(0, 2);
        editor.selection = new vscode.Selection(cursorPosition, movedCursorPosition);
        commands.deleteRight(editor, edit, -1);
        document.text = edit.text;
        assert.strictEqual(document.text, 's is some text    X');

        // remove second character 's' with continuation on column 18
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        commands.deleteRight(editor, edit, 18);
        document.text = edit.text;
        assert.strictEqual(document.text, ' is some text     X');
    });

    test('Cut chars test', () => {
        // prepare document and editor
        const document = new TextDocumentMock();
        const editor = new TextEditorMock(document);
        const cursorPosition = new vscode.Position(0, 0);
        editor.selection = new vscode.Selection(cursorPosition, cursorPosition);
        const edit = new TextEditorEditMock('this is some text    X');
        document.text = edit.text;

        // cutting without selection does nothing
        commands.cut(editor, edit, -1);
        document.text = edit.text;
        assert.strictEqual(document.text, 'this is some text    X');

        // remove first 2 characters 'th' without continuation
        const movedCursorPosition = new vscode.Position(0, 2);
        editor.selection = new vscode.Selection(cursorPosition, movedCursorPosition);
        commands.cut(editor, edit, -1);
        document.text = edit.text;
        assert.strictEqual(document.text, 'is is some text    X');

        // remove first 2 characters characters 'is' with continuation on column 19
        commands.cut(editor, edit, 19);
        document.text = edit.text;
        assert.strictEqual(document.text, ' is some text      X');
    });
});
