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

suite('Configurations Handler Test Suite', () => {
    const handler = new ConfigurationsHandler();
    const workspacePath = vscode.workspace.workspaceFolders[0].uri.fsPath;

    // configuration files paths
    test('Check configs test', () => {
        const configPaths = handler.checkConfigs();
        assert.equal(configPaths[0], path.join(workspacePath, '.hlasmplugin', 'pgm_conf.json'));
        assert.equal(configPaths[1], path.join(workspacePath, '.hlasmplugin', 'proc_grps.json'));
    });

    // 4 expressions - 2 for always recognize, 1 open code and 1 library file
    test('Update wildcards test', () => {
        const wildcards = handler.updateWildcards();
        assert.equal(wildcards.length, 7);
    });

    // 2 files matching the wildcards
    test('Check language test', () => {
        assert.ok(handler.match(path.join(workspacePath, 'file.asm')));
        assert.ok(handler.match(path.join(workspacePath, 'pgms/file')));
    });
});
