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

import { ConfigurationsHandler } from '../../configurationsHandler';
import { ebg_folder, bridge_json_file } from '../../constants';
import { retrieveConfigurationNodes } from '../../configurationNodes';
import { FileSystemMock } from '../mocks';

suite('Configurations Handler Test Suite', () => {
    const handler = new ConfigurationsHandler();
    const workspaceUri = vscode.workspace.workspaceFolders![0].uri;

    // 26 expressions - 2 for always recognize, 13 open codes (pgm_conf.json) and 11 library definitions (proc_grps.json)
    test('Update wildcards test', async () => {
        const wildcards = await handler.generateWildcards(workspaceUri);
        assert.ok(wildcards);
        assert.strictEqual(wildcards.length, 26);
    });

    // 2 files matching the wildcards
    test('Check language test', async () => {
        const wildcards = await handler.generateWildcards(workspaceUri);
        assert.ok(wildcards);
        handler.setWildcards(wildcards.map(regex => { return { regex, workspaceUri }; }));
        assert.ok(handler.match(vscode.Uri.joinPath(workspaceUri, 'file.asm')));
        assert.ok(handler.match(vscode.Uri.joinPath(workspaceUri, 'pgms/file')));
    });

    test('Existing b4g configs', async () => {
        const fsMock = new FileSystemMock();
        const pgmUri = vscode.Uri.joinPath(workspaceUri, "SYS/SUB/PGM");
        const bridgeJsonUri = vscode.Uri.joinPath(workspaceUri, "SYS/SUB", bridge_json_file);
        const ebgUri = vscode.Uri.joinPath(workspaceUri, ebg_folder);

        fsMock.addResource(bridgeJsonUri);
        fsMock.addResource(ebgUri);
        fsMock.addResource(pgmUri);

        const configNodes = await retrieveConfigurationNodes(workspaceUri, pgmUri, fsMock);
        assert.strictEqual(configNodes.bridgeJson.exists, true);
        assert.strictEqual(configNodes.ebgFolder.exists, true);
        assert.deepStrictEqual(configNodes.bridgeJson.uri, bridgeJsonUri);
        assert.deepStrictEqual(configNodes.ebgFolder.uri, ebgUri);
    });

    test('Non-existing b4g configs', async () => {
        const fsMock = new FileSystemMock();
        const pgmUri = vscode.Uri.joinPath(workspaceUri, "SYS/SUB/PGM");

        fsMock.addResource(pgmUri);

        const configNodes = await retrieveConfigurationNodes(workspaceUri, pgmUri, fsMock);
        assert.strictEqual(configNodes.bridgeJson.exists, false);
        assert.strictEqual(configNodes.ebgFolder.exists, false);
    });
});
