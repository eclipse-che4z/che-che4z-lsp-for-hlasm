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
import * as path from 'path';
import * as vscode from 'vscode';

import { ConfigurationsHandler } from '../../configurationsHandler';
import { hlasmplugin_folder, pgm_conf_file, proc_grps_file } from '../../constants';

suite('Configurations Handler Test Suite', () => {
    const handler = new ConfigurationsHandler();
    const workspaceUri = vscode.workspace.workspaceFolders[0].uri;

    // 20 expressions - 2 for always recognize, 9 open codes (pgm_conf.json) and 9 library definitions (proc_grps.json)
    test('Update wildcards test', async () => {
        const wildcards = await handler.generateWildcards(workspaceUri);
        assert.equal(wildcards.length, 20);
    });

    // 2 files matching the wildcards
    test('Check language test', async () => {
        handler.setWildcards((await handler.generateWildcards(workspaceUri)).map(regex => { return { regex, workspaceUri }; }));
        assert.ok(handler.match(vscode.Uri.joinPath(workspaceUri, 'file.asm')));
        assert.ok(handler.match(vscode.Uri.joinPath(workspaceUri, 'pgms/file')));
    });
});
