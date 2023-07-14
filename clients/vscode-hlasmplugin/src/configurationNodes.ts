/*
 * Copyright (c) 2023 Broadcom.
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
import { uriExists } from './helpers'
import { hlasmplugin_folder, proc_grps_file, pgm_conf_file, bridge_json_file, ebg_folder as ebg_folder } from './constants';

export interface ConfigurationNodeDetails {
    uri: vscode.Uri | typeof vscode.Uri;
    exists: boolean;
}

export interface ConfigurationNodes {
    procGrps: ConfigurationNodeDetails;
    pgmConf: ConfigurationNodeDetails;
    bridgeJson: ConfigurationNodeDetails;
    ebgFolder: ConfigurationNodeDetails;
}

export async function retrieveConfigurationNodes(workspace: vscode.Uri, documentUri: vscode.Uri | undefined, fs: vscode.FileSystem = vscode.workspace.fs): Promise<ConfigurationNodes> {
    const procGrps = vscode.Uri.joinPath(workspace, hlasmplugin_folder, proc_grps_file);
    const pgmConf = vscode.Uri.joinPath(workspace, hlasmplugin_folder, pgm_conf_file);
    const bridgeJson = documentUri ? vscode.Uri.joinPath(documentUri, "..", bridge_json_file) : undefined;
    const ebgFolder = vscode.Uri.joinPath(workspace, ebg_folder);
    return Promise.all([
        uriExists(procGrps, fs).then(b => { return { uri: procGrps, exists: b }; }),
        uriExists(pgmConf, fs).then(b => { return { uri: pgmConf, exists: b }; }),
        bridgeJson ? uriExists(bridgeJson, fs).then(b => { return { uri: bridgeJson, exists: b }; }) : { uri: vscode.Uri, exists: false },
        uriExists(ebgFolder, fs).then(b => { return { uri: ebgFolder, exists: b }; }),
    ]).then(arr => { return { procGrps: arr[0], pgmConf: arr[1], bridgeJson: arr[2], ebgFolder: arr[3] }; });
}

export function anyConfigurationNodeExists(configNodes: ConfigurationNodes) {
    return configNodes.procGrps.exists || configNodes.pgmConf.exists || configNodes.bridgeJson.exists || configNodes.ebgFolder.exists;
}
