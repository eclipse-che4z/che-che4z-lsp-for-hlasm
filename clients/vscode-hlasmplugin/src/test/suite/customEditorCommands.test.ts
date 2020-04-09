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

suite('Language Detection Test Suite', () => {
    const commands = new CustomEditorCommands();

	test('Insert chars test', () => {
        const document = new TextDocumentMock();
        const editor = new TextEditorMock(document);
        const cursorPosition = new vscode.Position(0,0);
        editor.selection = new vscode.Selection(cursorPosition,cursorPosition);
        const edit = new TextEditorEditMock('this is some text    X');
        // add 'a' at the beginning of text without continuation
        commands.insertChars(editor,edit,'a', -1);
        document.text = edit.text;
        assert.equal(edit.text,'athis is some text    X');

        // replace 'at' for 'bc' without continuation
        const movedCursorPosition = new vscode.Position(0,2);
        editor.selection = new vscode.Selection(cursorPosition,movedCursorPosition);
        commands.insertChars(editor,edit,'bc', -1);
        document.text = edit.text;
        assert.equal(edit.text,'bchis is some text    X');

        // simulate continuation on column 22
        // add 'd' at the beginning
        editor.selection = new vscode.Selection(cursorPosition,cursorPosition);
        commands.insertChars(editor,edit,'d', 22);
        document.text = edit.text;
        assert.equal(edit.text,'dbchis is some text   X');
    });
    
    test('Delete chars test', () => {

	});
});