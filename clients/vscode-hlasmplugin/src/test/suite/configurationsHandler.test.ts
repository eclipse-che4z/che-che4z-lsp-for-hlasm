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
import { configurationExists } from '../../helpers';
import { FileSystemMock } from '../mocks';

suite('Configurations Handler Test Suite', () => {
    const handler = new ConfigurationsHandler();
    const workspaceUri = vscode.workspace.workspaceFolders[0].uri;

    // 22 expressions - 2 for always recognize, 10 open codes (pgm_conf.json) and 10 library definitions (proc_grps.json)
    test('Update wildcards test', async () => {
        const wildcards = await handler.generateWildcards(workspaceUri);
        assert.equal(wildcards.length, 22);
    });

    // 2 files matching the wildcards
    test('Check language test', async () => {
        handler.setWildcards((await handler.generateWildcards(workspaceUri)).map(regex => { return { regex, workspaceUri }; }));
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

        const [g, p, b, e] = await configurationExists(workspaceUri, pgmUri, fsMock);
        assert.equal(b.exists, true);
        assert.equal(e.exists, true);
        assert.deepStrictEqual(b.uri, bridgeJsonUri);
        assert.deepStrictEqual(e.uri, ebgUri);
    });

    test('Non-existing b4g configs', async () => {
        const fsMock = new FileSystemMock();
        const pgmUri = vscode.Uri.joinPath(workspaceUri, "SYS/SUB/PGM");

        fsMock.addResource(pgmUri);

        const [g, p, b, e] = await configurationExists(workspaceUri, pgmUri, fsMock);
        assert.equal(b.exists, false);
        assert.equal(e.exists, false);
    });
});
