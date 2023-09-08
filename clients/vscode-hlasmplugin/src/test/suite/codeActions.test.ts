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
import { isCancellationError } from '../../helpers';
import { hlasmplugin_folder, pgm_conf_file, bridge_json_file } from '../../constants';

async function queryCodeActions(uri: vscode.Uri, range: vscode.Range, sleep: number, attempts: number = 10) {
    for (let i = 0; i < attempts; ++i) {
        // it seems that vscode occasionally makes its own request and cancels ours
        await helper.sleep(sleep);

        try {
            const codeActionsList: vscode.CodeAction[] = await vscode.commands.executeCommand('vscode.executeCodeActionProvider', uri, range);

            // empty list also points towards the request being cancelled
            if (codeActionsList.length === 0)
                continue;

            if (i > 0)
                console.log(`Code actions required ${i + 1} attempts`);

            return codeActionsList;
        } catch (e) {
            assert.ok(isCancellationError(e));
        }
    }
    throw Error("Code actions query failed");
}

suite('Code actions', () => {
    suiteSetup(async function () {
        this.timeout(20000);
    });

    suiteTeardown(async function () {
        await helper.closeAllEditors();
    });

    test('Diagnostics not suppressed', async () => {
        const file = 'code_action_1.hlasm';

        const diagnostic_event = helper.waitForDiagnostics(file);

        const { editor, document } = await helper.showDocument(file, 'hlasm');

        await diagnostic_event;

        const codeActionsList = await queryCodeActions(document.uri, new vscode.Range(0, 10, 0, 15), 500);

        assert.strictEqual(codeActionsList.length, 4 + 3);

        await helper.closeAllEditors();
    }).timeout(10000).slow(5000);

    test('Diagnostics suppressed', async () => {
        const file = 'code_action_2.hlasm';
        const diagnostic_event = helper.waitForDiagnostics(file);

        const { editor, document } = await helper.showDocument(file, 'hlasm');

        await diagnostic_event;

        const codeActionsList = await queryCodeActions(document.uri, new vscode.Range(0, 10, 0, 15), 500);

        assert.strictEqual(codeActionsList.length, 1);

        await helper.closeAllEditors();
    }).timeout(10000).slow(5000);

    async function configurationDiagnosticsHelper(file: string, configFileUri: vscode.Uri, errorDiags: (string)[], allDiags: (string)[], diagSource: string) {
        const configRelPath = vscode.workspace.asRelativePath(configFileUri);
        let diags = await helper.waitForDiagnosticsChange(configRelPath, async () => { await helper.showDocument(file, 'hlasm'); }, diagSource);

        helper.assertMatchingMessageCodes(diags, errorDiags, diagSource);

        let codeActionsList = await queryCodeActions(configFileUri, new vscode.Range(0, 0, 0, 0), 500)
            .then(codeActionList => codeActionList.filter(x => x.command?.command === 'extension.hlasm-plugin.toggleAdvisoryConfigurationDiagnostics'));

        assert.strictEqual(codeActionsList.length, 1);
        assert.strictEqual(codeActionsList[0].command!.title, 'Show all configuration diagnostics');

        diags = await helper.waitForDiagnosticsChange(configRelPath, () => { vscode.commands.executeCommand(codeActionsList[0].command!.command, configFileUri, new vscode.Range(0, 0, 0, 0)); }, diagSource);

        helper.assertMatchingMessageCodes(diags, allDiags, diagSource);

        codeActionsList = await queryCodeActions(configFileUri, new vscode.Range(0, 0, 0, 0), 500).then(codeActionList => codeActionList.filter(x => x.command?.command === 'extension.hlasm-plugin.toggleAdvisoryConfigurationDiagnostics'));

        assert.strictEqual(codeActionsList.length, 1);
        assert.strictEqual(codeActionsList[0].command!.title, 'Don\'t show advisory configuration diagnostics');

        diags = await helper.waitForDiagnosticsChange(configRelPath, () => { vscode.commands.executeCommand(codeActionsList[0].command!.command, configFileUri, new vscode.Range(0, 0, 0, 0)); }, diagSource);

        helper.assertMatchingMessageCodes(diags, errorDiags, diagSource);
    }

    test('Missing processor groups - pgm_conf.json', async () => {
        const pgmConf = await helper.showDocument(`${hlasmplugin_folder}/${pgm_conf_file}`);

        await configurationDiagnosticsHelper('missing_pgroup/A.hlasm', pgmConf.document.uri, ['W0004'], ['W0004', 'W0008'], 'HLASM Plugin');

        await helper.closeAllEditors();
    }).timeout(15000).slow(10000);

    test('Missing processor groups - .bridge.json', async () => {
        const bridgeJson = await helper.showDocument(`missing_pgroup/b4g/${bridge_json_file}`);

        await configurationDiagnosticsHelper(`missing_pgroup/b4g/A`, bridgeJson.document.uri, ['B4G002'], ['B4G002', 'B4G003'], 'HLASM Plugin');

        await helper.closeAllEditors();
    }).timeout(15000).slow(10000);
});
