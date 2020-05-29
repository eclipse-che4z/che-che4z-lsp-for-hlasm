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
describe('Integration Test Suite for Theia', function () {
    const theia = require('@theia/plugin');

    const Uri = require('@theia/core/lib/common/uri');
    const {assert} = chai;
    const { EditorManager } = require('@theia/editor/lib/browser/editor-manager');
    const { WorkspaceService } = require('@theia/workspace/lib/browser/workspace-service');
    const { MonacoEditor } = require('@theia/monaco/lib/browser/monaco-editor');

    /** @type {import('inversify').Container} */
    const container = window['theia'].container;
    const editorManager = container.get(EditorManager);
    const workspaceService = container.get(WorkspaceService);
    const workspaceRoot = workspaceService.tryGetRoots()[0];
    /** @type {MonacoEditor} */
    let monacoEditor;
    // open 'open' file, should be recognized as hlasm
    it('HLASM file open test', function (done) {
        console.log(theia.languages.getDiagnostics());
        editorManager.open(new Uri.default(workspaceRoot.uri).resolve('open'), {
            mode: 'reveal'
        }).then(editor => {
            assert.ok(editor);
            monacoEditor = MonacoEditor.get(editor);
            assert.ok(monacoEditor);
            setTimeout(function () {
                if (monacoEditor.document.languageId != 'hlasm')
                    done('Wrong language');
                else
                    done();
            }, 1000);
        });
    }).timeout(10000).slow(4000);

    // change 'open' file to create diagnostic
    it('Diagnostic test', (done) => {
        assert.equal(vscode.window.activeTextEditor, openFileEditor);
        // register callback to check for the correctness of the diagnostic
        const listener = vscode.languages.onDidChangeDiagnostics(_ => {
            const diags = vscode.languages.getDiagnostics();
            listener.dispose();
            if (diags.length, 1 && diags[0][1][0].code == 'M003')
                done();
            else
                done('Wrong diagnostic');
        });
        // remove second parameter from LR instruction
        openFileEditor.edit(edit => {
            edit.delete(new vscode.Range(new vscode.Position(2, 6), new vscode.Position(2, 7)));
        });
    }).timeout(10000).slow(1000);
});