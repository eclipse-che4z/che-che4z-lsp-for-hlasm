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

import { EventsHandler } from '../../eventsHandler';
import {
    LanguageClientMock, LanguageClientOptionsMock,
    TextEditorMock, TextDocumentMock, ConfigurationChangeEventMock,
    TextDocumentChangeEventMock, TextDocumentContentChangeEventMock,
} from '../mocks';

suite('Events Handler Test Suite', () => {
    const options = new LanguageClientOptionsMock();
    const client = new LanguageClientMock('client', 'mock', options);
    const handler = new EventsHandler('editor.action.triggerSuggest');

    test('Active Text Editor Change test', async () => {
        const document = new TextDocumentMock();
        document.languageId = 'hlasm';
        document.uri = vscode.Uri.file('hlasm')
        const editor = new TextEditorMock(document);
        await handler.onDidChangeActiveTextEditor(editor);
    });

    test('Configuration Change test', () => {
        const event = new ConfigurationChangeEventMock();
        handler.onDidChangeConfiguration(event);
    });

    test('Text Document Change test', () => {
        assert.ok(vscode.workspace.getConfiguration('hlasm').get('continuationHandling'));
        // prepare document
        const document = new TextDocumentMock();
        document.languageId = 'hlasm';
        document.text = ' ';
        document.uri = vscode.Uri.file('file');
        // prepare event 
        const change = new TextDocumentContentChangeEventMock();
        const position = new vscode.Position(0, 0);
        change.range = new vscode.Range(position, position);
        const event = new TextDocumentChangeEventMock([change]);
        event.document = document;
        // get completion results
        assert.ok(handler.onDidChangeTextDocument(event, 71));
    });

    test('Text Document Open test', async () => {
        // prepare highlighting feature
        const options = new LanguageClientOptionsMock();
        const client = new LanguageClientMock('client', 'mock', options);
        // prepare document
        const document = new TextDocumentMock();
        document.languageId = 'hlasm';
        document.uri = vscode.Uri.file('hlasm')
        await handler.onDidOpenTextDocument(document);
    });

    test('Text Document Saved test', async () => {
        // prepare document
        const document = new TextDocumentMock();
        document.fileName = 'file';
        document.uri = vscode.Uri.file('file')
        await handler.onDidSaveTextDocument(document);
    })
});
