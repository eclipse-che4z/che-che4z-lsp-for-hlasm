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
import { LanguageClientMock, LanguageClientOptionsMock, 
    TextEditorMock, TextDocumentMock, ConfigurationChangeEventMock, 
    TextDocumentChangeEventMock, TextDocumentContentChangeEventMock,SemanticTokensFeatureMock } from '../mocks';

suite('Events Handler Test Suite', () => {
    const options = new LanguageClientOptionsMock();
        const client = new LanguageClientMock('client', 'mock', options);
    const highlight = new SemanticTokensFeatureMock(client);
    const handler = new EventsHandler('editor.action.triggerSuggest',highlight);

    test('Active Text Editor Change test', () => {
        const document = new TextDocumentMock();
        document.languageId = 'hlasm';
        const editor = new TextEditorMock(document);
        handler.onDidChangeActiveTextEditor(editor);
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
        assert.ok(handler.onDidChangeTextDocument(event,highlight,71));
    });

    test('Visible Text Editors Change test', () => {
        // prepare document
        const document = new TextDocumentMock();
        document.languageId = 'hlasm';
        // prepare editor
        const editor = new TextEditorMock(document);
        // prepare highlighting feature
        const options = new LanguageClientOptionsMock();
        const client = new LanguageClientMock('client', 'mock', options);
        const highlight = new SemanticTokensFeatureMock(client);
        handler.onDidChangeVisibleTextEditors([editor], highlight);
        assert.ok(highlight.didColorize);
    });

    test('Text Document Open test', () => {
        // prepare highlighting feature
        const options = new LanguageClientOptionsMock();
        const client = new LanguageClientMock('client', 'mock', options);
        const highlight = new SemanticTokensFeatureMock(client);
        // prepare document
        const document = new TextDocumentMock();
        document.languageId = 'hlasm';
        handler.onDidOpenTextDocument(document,highlight);
    });

    test('Text Document Saved test', () => {
        // prepare document
        const document = new TextDocumentMock();
        document.fileName = 'file';
        handler.onDidSaveTextDocument(document);
    })
});
