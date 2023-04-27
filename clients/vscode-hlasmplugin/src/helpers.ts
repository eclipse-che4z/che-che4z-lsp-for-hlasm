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
import { hlasmplugin_folder, proc_grps_file, pgm_conf_file, bridge_json_file, ebg_folder } from './constants';

export async function uriExists(uri: vscode.Uri, fs: vscode.FileSystem = vscode.workspace.fs): Promise<boolean> {
    return fs.stat(uri).then(() => { return true; }, () => { return false; })
}

export async function configurationExists(workspace: vscode.Uri, documentUri: vscode.Uri | undefined, fs: vscode.FileSystem = vscode.workspace.fs) {
    const procGrps = vscode.Uri.joinPath(workspace, hlasmplugin_folder, proc_grps_file);
    const pgmConf = vscode.Uri.joinPath(workspace, hlasmplugin_folder, pgm_conf_file);
    const bridgeJson = documentUri ? vscode.Uri.joinPath(documentUri, "..", bridge_json_file) : undefined;
    const ebgPath = vscode.Uri.joinPath(workspace, ebg_folder);
    return Promise.all([
        uriExists(procGrps, fs).then(b => { return { uri: procGrps, exists: b }; }),
        uriExists(pgmConf, fs).then(b => { return { uri: pgmConf, exists: b }; }),
        bridgeJson ? uriExists(bridgeJson, fs).then(b => { return { uri: bridgeJson, exists: b }; }) : { uri: vscode.Uri, exists: false },
        uriExists(ebgPath, fs).then(b => { return { uri: ebgPath, exists: b }; }),
    ]);
}

export function isCancellationError(e: any) {
    return e instanceof vscode.CancellationError || e instanceof Error && e.message == new vscode.CancellationError().message;
}
