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
import * as path from 'path';
import * as helper from './testHelper';
import { waitForDiagnostics } from './testHelper';

suite('Integration Test Suite', () => {
    const workspace_file = 'open';
    let editor: vscode.TextEditor;

    suiteSetup(async function () {
        this.timeout(30000);

        editor = (await helper.showDocument(workspace_file)).editor;
    });

    suiteTeardown(async function () {
        await helper.closeAllEditors();
    });

    // open 'open' file, should be recognized as hlasm
    test('HLASM file open test', async () => {
        // setting a language takes a while but shouldn't take longer than a second
        await helper.sleep(1000);
        assert.strictEqual(editor.document.languageId, 'hlasm');
    }).timeout(10000).slow(4000);

    // change 'open' file to create diagnostic
    test('Diagnostic test', async () => {
        // register callback to check for the correctness of the diagnostic
        const diagnostic_event = helper.waitForDiagnostics(workspace_file, true);
        // remove second parameter from LR instruction
        await editor.edit(edit => {
            edit.delete(new vscode.Range(new vscode.Position(2, 6), new vscode.Position(2, 7)));
        });

        const diags = await diagnostic_event;
        const codes = diags.map(x => x.code || '');
        assert.deepStrictEqual(codes, ['M003'], editor.document.getText());
    }).timeout(10000).slow(1000);

    async function insertBestCompletion() {
        // for some reason insertBestCompletion does not do anything
        await vscode.commands.executeCommand('editor.action.triggerSuggest');
        await helper.sleep(1000);

        await vscode.commands.executeCommand('acceptSelectedSuggestion');
        await helper.sleep(1000);
    }

    // test completion for instructions
    test('Completion Instructions test', async () => {
        await helper.insertString(editor, new vscode.Position(7, 1), 'L');

        await insertBestCompletion();

        const acceptedLine = editor.document.getText(new vscode.Range(7, 0, 8, 0));

        assert.match(acceptedLine, /L             R1,D12U2\(X2,B2\)/, 'Wrong suggestion result');
    }).timeout(10000).slow(4000);

    // test completion for variable symbols
    test('Completion Variable symbol test', async () => {
        await helper.insertString(editor, new vscode.Position(8, 0), '&');

        await insertBestCompletion();

        const acceptedLine = editor.document.getText(new vscode.Range(8, 0, 9, 0));

        assert.match(acceptedLine, /&VAR/, 'Wrong suggestion result');
    }).timeout(10000).slow(4000);

    // go to definition for ordinary symbol
    test('Definition Ordinary symbol test', async () => {
        const result: vscode.Location[] = await vscode.commands.executeCommand('vscode.executeDefinitionProvider', editor.document.uri, new vscode.Position(1, 7));

        assert.strictEqual(result.length, 1, 'Wrong ordinary symbol definition count');
        assert.strictEqual(result[0].uri.fsPath, editor.document.fileName, 'Wrong ordinary symbol definition filename');
        assert.strictEqual(result[0].range.start.line, 9, 'Wrong ordinary symbol definition line');
        assert.strictEqual(result[0].range.start.character, 0, 'Wrong ordinary symbol definition column');
    }).timeout(10000).slow(1000);

    // hover for variable symbol
    test('Hover Variable symbol test', async () => {
        const result: vscode.Hover[] = await vscode.commands.executeCommand('vscode.executeHoverProvider', editor.document.uri, new vscode.Position(6, 8));

        assert.strictEqual(result.length, 1);
        assert.strictEqual(result[0].contents.length, 1);
        assert.strictEqual((result[0].contents[0] as vscode.MarkdownString).value, 'SETA variable', 'Wrong variable symbol hover contents');
    }).timeout(10000).slow(1000);

    // go to definition for macros
    test('Definition Macro test', async () => {
        const result: vscode.Location[] = await vscode.commands.executeCommand('vscode.executeDefinitionProvider', editor.document.uri, new vscode.Position(6, 2));
        assert.strictEqual(result.length, 1, 'Wrong ordinary symbol definition count');
        assert.strictEqual(result[0].uri.fsPath, path.join(helper.getWorkspacePath(), 'libs', 'mac.asm'), 'Wrong ordinary symbol definition filename');
        assert.strictEqual(result[0].range.start.line, 1, 'Wrong ordinary symbol definition line');
        assert.strictEqual(result[0].range.start.character, 4, 'Wrong ordinary symbol definition column');
    }).timeout(10000).slow(1000);

    // debug open code test
    test('Debug test', async () => {
        const session = await helper.debugStartSession();

        // step over once
        await helper.debugStepOver(1);
        // then check for VAR2 variable
        const scopesResult = await session.customRequest('scopes', { frameId: 0 });

        const scopes = scopesResult.body ? scopesResult.body.scopes : scopesResult.scopes;

        const reference = scopes.find((scope: { name: string }) => scope.name == 'Locals').variablesReference;
        const variablesResult = await session.customRequest('variables', { variablesReference: reference });

        const variables = variablesResult.body ? variablesResult.body.variables : variablesResult.variables;

        assert.strictEqual(variables.length, 1);
        assert.strictEqual(variables[0].value, 'SOMETHING', 'Wrong debug variable &VAR2');
        assert.strictEqual(variables[0].name, '&VAR2', 'Wrong debug variable &VAR2');

        await helper.debugStop();
    }).timeout(20000).slow(10000);

    async function openDocumentAndCheckDiags(workspace_file: string) {
        const diagsChange = waitForDiagnostics(workspace_file, true);
        const uri = (await helper.showDocument(workspace_file)).document.uri.toString();

        const diags = await diagsChange;

        assert.ok(diags);
        assert.strictEqual(diags.length, 1);
        assert.strictEqual(diags[0].code, 'MNOTE');
        assert.strictEqual(diags[0].message, 'DONE', "Library patterns are not working for file: " + workspace_file);
    }

    // verify that library patterns are working
    test('General', async () => {
        await openDocumentAndCheckDiags("pattern_test/test_pattern.hlasm");
    }).timeout(10000).slow(2500);

    test('Special chars - basic character set', async () => {
        await openDocumentAndCheckDiags("pattern_test/!#$%&'()+,-.0123456789;=@ABCDEFGHIJKLMNOPQRSTUVWXYZ??^_`abcdefghijklmnopqrstuvwxyz??~.hlasm");
    }).timeout(10000).slow(2500);

    test('1 Byte UTF-8 Encoding', async () => {
        await openDocumentAndCheckDiags("pattern_test/test_utf_8_+.hlasm");
    }).timeout(10000).slow(2500);

    test('2 Byte UTF-8 Encoding', async () => {
        await openDocumentAndCheckDiags("pattern_test/test_utf_8_߿.hlasm");
    }).timeout(10000).slow(2500);

    test('3 Byte UTF-8 Encoding', async () => {
        await openDocumentAndCheckDiags("pattern_test/test_utf_8_ﾝ.hlasm");
    }).timeout(10000).slow(2500);

    test('4 Byte UTF-8 Encoding', async () => {
        await openDocumentAndCheckDiags("pattern_test/test_utf_8_🧿.hlasm");
    }).timeout(10000).slow(2500);

    test('Wildcards and UTF-8 Encoding (Part #1)', async () => {
        await openDocumentAndCheckDiags("pattern_test/$test㎛_utf🧽_8_߽.hlasm");
    }).timeout(10000).slow(2500);

    test('Wildcards and UTF-8 Encoding (Part #2)', async () => {
        await openDocumentAndCheckDiags("pattern_test/test¾_🧼utf@_8_☕.hlasm");
    }).timeout(10000).slow(2500);
});
