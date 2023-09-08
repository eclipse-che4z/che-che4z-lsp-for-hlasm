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
import { ConfigurationNodeDetails, ConfigurationNodes, anyConfigurationNodeExists } from '../configurationNodes';
import { proc_grps_file, pgm_conf_file } from '../constants';

function codeActionFactory(create: boolean, filename: string | undefined, args: any[] | undefined): vscode.CodeAction {
    const filesDescription: string = filename ? filename + ' configuration file' : 'configuration files';

    return {
        title: (create ? 'Create ' : 'Modify ') + filesDescription,
        command: {
            title: (create ? 'Create ' : 'Open ') + filesDescription,
            command: create ? 'extension.hlasm-plugin.createCompleteConfig' : 'vscode.open',
            arguments: args
        },
        kind: vscode.CodeActionKind.QuickFix
    };
}

function generateProcGrpsCodeAction(procGrps: ConfigurationNodeDetails, wsUri: vscode.Uri): vscode.CodeAction {
    return procGrps.exists ? codeActionFactory(false, proc_grps_file, [procGrps.uri]) : codeActionFactory(true, proc_grps_file, [wsUri, null, '']);
}

function generatePgmConfCodeAction(configNodes: ConfigurationNodes, wsUri: vscode.Uri, documentRelativeUri: string): vscode.CodeAction {
    if (configNodes.pgmConf.exists)
        return codeActionFactory(false, pgm_conf_file, [configNodes.pgmConf.uri]);
    else {
        if (configNodes.bridgeJson.exists || configNodes.ebgFolder.exists) {
            // TODO: could we trigger B4G sync?
        }
        return codeActionFactory(true, pgm_conf_file, [wsUri, documentRelativeUri, null]);
    }
}

export function generateConfigurationFilesCodeActions(suggestProcGrpsChange: boolean, suggestPgmConfChange: boolean, configNodes: ConfigurationNodes, wsUri: vscode.Uri, documentRelativeUri: string): vscode.CodeAction[] {
    if (!suggestProcGrpsChange && !suggestPgmConfChange)
        return [];

    if (!anyConfigurationNodeExists(configNodes))
        return [codeActionFactory(true, undefined, [wsUri, documentRelativeUri, 'GRP1'])];

    const result: vscode.CodeAction[] = [];

    if (suggestProcGrpsChange)
        result.push(generateProcGrpsCodeAction(configNodes.procGrps, wsUri));

    if (suggestPgmConfChange)
        result.push(generatePgmConfCodeAction(configNodes, wsUri, documentRelativeUri));

    return result;
}
