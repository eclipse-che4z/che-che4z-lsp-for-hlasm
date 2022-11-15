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
import * as vscode from 'vscode';
import { hlasmplugin_folder, proc_grps_file, pgm_conf_file, ebg_folder } from './constants';

export async function uriExists(uri: vscode.Uri): Promise<boolean> {
    return vscode.workspace.fs.stat(uri).then(() => { return true; }, () => { return false; })
}

export async function configurationExists(workspace: vscode.Uri) {
    const procGrps = vscode.Uri.joinPath(workspace, hlasmplugin_folder, proc_grps_file);
    const pgmConf = vscode.Uri.joinPath(workspace, hlasmplugin_folder, pgm_conf_file);
    const ebgPath = vscode.Uri.joinPath(workspace, ebg_folder);
    return Promise.all([
        uriExists(procGrps).then(b => { return { uri: procGrps, exists: b }; }),
        uriExists(pgmConf).then(b => { return { uri: pgmConf, exists: b }; }),
        uriExists(ebgPath).then(b => { return { uri: ebgPath, exists: b }; }),
    ]);
}
