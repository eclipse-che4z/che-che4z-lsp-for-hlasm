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
import { ConfigurationNodeDetails, ConfigurationNodes, ConfigurationNodesWithoutWorkspace, SettingsConfigurationNodeDetails, SettingsConfigurationNodeDetailsWithoutWorkspace, anyConfigurationNodeExists } from '../configurationNodes';

function filenameFromUri(uri: vscode.Uri): string {
    return uri.path.substring(uri.path.lastIndexOf('/') + 1);
}

function createAllConfigurationFiles(ws: vscode.Uri, relativeUri: string, pgroup: string): vscode.CodeAction {
    return {
        title: 'Create configuration files',
        command: {
            title: 'Create configuration files',
            command: 'extension.hlasm-plugin.createCompleteConfig',
            arguments: [ws, relativeUri, pgroup],
        },
        kind: vscode.CodeActionKind.QuickFix
    };
}

function createConfigurationFile(filename: string, ws: vscode.Uri, relativeUri: string | null, pgroup: string | null): vscode.CodeAction {
    const filesDescription: string = filename + ' configuration file';
    return {
        title: 'Create ' + filesDescription,
        command: {
            title: 'Create ' + filesDescription,
            command: 'extension.hlasm-plugin.createCompleteConfig',
            arguments: [ws, relativeUri, pgroup],
        },
        kind: vscode.CodeActionKind.QuickFix
    };
}

function modifyConfigurationFile(uri: vscode.Uri): vscode.CodeAction {
    const filename = filenameFromUri(uri);
    const filesDescription: string = filename + ' configuration file';

    return {
        title: 'Modify ' + filesDescription,
        command: {
            title: 'Open ' + filesDescription,
            command: 'vscode.open',
            arguments: [uri]
        },
        kind: vscode.CodeActionKind.QuickFix
    };
}

function modifySettings(s: SettingsConfigurationNodeDetails | SettingsConfigurationNodeDetailsWithoutWorkspace): vscode.CodeAction {
    const title = `Modify settings key '${s.key}'`;
    if (s.scope === vscode.ConfigurationTarget.WorkspaceFolder)
        return {
            title,
            command: {
                title,
                command: 'vscode.open',
                arguments: [vscode.Uri.joinPath(s.ws, '.vscode/settings.json')],
            },
        };
    else
        return {
            title,
            command: {
                title,
                command: s.scope === vscode.ConfigurationTarget.Workspace ? 'workbench.action.openWorkspaceSettingsFile' : 'workbench.action.openSettingsJson',
                arguments: [{
                    revealSetting: {
                        key: s.key,
                        edit: true,
                    }
                }],
            },
        };
}

function generateProcGrpsCodeAction(procGrps: ConfigurationNodeDetails | SettingsConfigurationNodeDetails, wsUri: vscode.Uri): vscode.CodeAction {
    if (procGrps.exists)
        if ('uri' in procGrps)
            return modifyConfigurationFile(procGrps.uri);
        else
            return modifySettings(procGrps);
    else
        return createConfigurationFile(filenameFromUri(procGrps.uri), wsUri, null, '');
}

function generatePgmConfCodeAction(configNodes: ConfigurationNodes, wsUri: vscode.Uri, documentRelativeUri: string): vscode.CodeAction {
    if (configNodes.pgmConf.exists) {
        if ('uri' in configNodes.pgmConf)
            return modifyConfigurationFile(configNodes.pgmConf.uri);
        else
            return modifySettings(configNodes.pgmConf);
    }
    else {
        if (configNodes.bridgeJson.exists || configNodes.ebgFolder.exists) {
            // TODO: could we trigger B4G sync?
        }
        return createConfigurationFile(filenameFromUri(configNodes.pgmConf.uri), wsUri, documentRelativeUri, null);
    }
}

export function generateConfigurationFilesCodeActions(suggestProcGrpsChange: boolean, suggestPgmConfChange: boolean, config: { nodes: ConfigurationNodes, ws: vscode.Uri, documentRelativeUri: string } | { nodes: ConfigurationNodesWithoutWorkspace }): vscode.CodeAction[] {
    if (!suggestProcGrpsChange && !suggestPgmConfChange)
        return [];

    if (!('ws' in config))
        return [modifySettings(config.nodes.procGrps), modifySettings(config.nodes.pgmConf)];

    if (!anyConfigurationNodeExists(config.nodes))
        return [createAllConfigurationFiles(config.ws, config.documentRelativeUri, 'GRP1')];

    const result: vscode.CodeAction[] = [];

    if (suggestProcGrpsChange)
        result.push(generateProcGrpsCodeAction(config.nodes.procGrps, config.ws));

    if (suggestPgmConfChange)
        result.push(generatePgmConfCodeAction(config.nodes, config.ws, config.documentRelativeUri));

    return result;
}
