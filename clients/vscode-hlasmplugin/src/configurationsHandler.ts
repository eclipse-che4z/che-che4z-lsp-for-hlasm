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

import * as vscode from 'vscode';
import { hlasmplugin_folder, proc_grps_file, pgm_conf_file } from './constants';
import { retrieveConfigurationNodes } from './configurationNodes';
import { generateConfigurationFilesCodeActions } from './code_actions/configurationFilesActions';
import { textDecode } from './tools.common';

/**
 * Handles changes in configurations files.
 * Invokes configuration prompts in case they are missing.
 * Stores and checks wildcards for file matching.
 */
export class ConfigurationsHandler {
    // defined regex expression to match files and recognize them as HLASM
    private definedExpressions: { regex: RegExp, workspaceUri: vscode.Uri }[] = [];

    constructor() {
        this.definedExpressions = [];
    }

    /**
     * Checks whether the given path matches any of the wildcards
     * If so, it is HLASM
     */
    match(file: vscode.Uri): boolean {
        return this.definedExpressions.find(expr => expr.regex.test(file.toString())) !== undefined;
    }

    updateWildcards(workspaceUri: vscode.Uri, matches: RegExp[]) {
        this.definedExpressions = this.definedExpressions.filter(x => x.workspaceUri !== workspaceUri).concat(matches.map(regex => { return { regex, workspaceUri }; }));
    }

    setWildcards(matches: { regex: RegExp, workspaceUri: vscode.Uri }[]) {
        this.definedExpressions = matches;
    }

    // update wildcards when pgm conf changes (on save)
    async generateWildcards(workspaceUri: vscode.Uri, reloadedFile?: vscode.Uri): Promise<RegExp[] | null> {
        const procGrps = vscode.Uri.joinPath(workspaceUri, hlasmplugin_folder, proc_grps_file);
        const pgmConf = vscode.Uri.joinPath(workspaceUri, hlasmplugin_folder, pgm_conf_file);
        // the reloaded file is not a config file
        if (reloadedFile && reloadedFile != pgmConf && reloadedFile != procGrps)
            return null;
        // configs do not exist
        if (!await ConfigurationsHandler.configFilesExist(workspaceUri))
            return [];

        //clear expressions
        const definedExpressions: RegExp[] = [];
        // get user-defined wildcards
        let content = JSON.parse(textDecode(await vscode.workspace.fs.readFile(pgmConf)));

        // convert each pgm to regex
        if (content.pgms) {
            (content.pgms as any[]).forEach(pgm => {
                const regex = this.convertWildcardToRegex(vscode.Uri.joinPath(workspaceUri, pgm.program as string).toString());
                if (regex)
                    definedExpressions.push(regex);
            });
        }

        // convert each wildcard to regex
        if (content.alwaysRecognize) {
            (content.alwaysRecognize as string[]).forEach(strExpr => {
                const regex = this.convertWildcardToRegex(vscode.Uri.joinPath(workspaceUri, strExpr).toString());
                if (regex)
                    definedExpressions.push(regex);
            });
        }

        content = JSON.parse(textDecode(await vscode.workspace.fs.readFile(procGrps)));
        // convert each pgroup library path to regex
        if (content.pgroups) {
            (content.pgroups as any[]).forEach(pgroup => {
                if (pgroup.libs)
                    (pgroup.libs as any[]).forEach(lib => {
                        if (typeof lib !== 'string' && typeof lib?.path !== 'string') return;
                        const regex = this.convertWildcardToRegex(vscode.Uri.joinPath(workspaceUri, lib.path || lib, '*').toString());
                        if (regex)
                            definedExpressions.push(regex);
                    })
            });
        }
        return definedExpressions;
    }

    public static async createPgmTemplate(programName: string, workspace: vscode.Uri, group_name: string = ''): Promise<vscode.Uri> {
        const folder = vscode.Uri.joinPath(workspace, hlasmplugin_folder)
        await vscode.workspace.fs.createDirectory(folder);
        const pgmConf = vscode.Uri.joinPath(folder, pgm_conf_file);

        await vscode.workspace.fs.writeFile(pgmConf, new TextEncoder().encode(JSON.stringify(
            {
                "pgms": [
                    { "program": programName, "pgroup": group_name }
                ]
            }
            , null, 2)
        ));

        return pgmConf;
    }

    public static async createProcTemplate(workspace: vscode.Uri, group_name: string = ''): Promise<vscode.Uri> {
        const folder = vscode.Uri.joinPath(workspace, hlasmplugin_folder)
        await vscode.workspace.fs.createDirectory(folder);
        const procGrps = vscode.Uri.joinPath(folder, proc_grps_file);

        await vscode.workspace.fs.writeFile(procGrps, new TextEncoder().encode(JSON.stringify(
            {
                "pgroups": [
                    { "name": group_name, "libs": [] }
                ]
            }, null, 2)
        ));

        return procGrps;
    }

    // converts wildcards to regexes
    private convertWildcardToRegex(wildcard: string): RegExp {
        return new RegExp('^' +
            wildcard.replace(/\(|\[|\{|\\|\^|\-|\=|\$|\!|\||\]|\}|\)|\./g, (char) => { return "\\" + char })
                .replace(/%3[fF]/g, ".")
                .replace(/%2[aA]/g, (char) => { return ".*?"; })
                .replace(/%2[bB]/g, (char) => { return ".+?"; })
            + '$');
    }

    /**
     * Checks if the configs are there and stores their complete paths
     */
    public static async configFilesExist(workspace: vscode.Uri): Promise<boolean> {
        const configNodes = await retrieveConfigurationNodes(workspace, undefined);
        return configNodes.procGrps.exists && configNodes.pgmConf.exists;
    }

    public static createCompleteConfig(workspace: vscode.Uri, program: string, group: string) {
        if (program)
            ConfigurationsHandler.createPgmTemplate(program, workspace, group || '').then((uri) => vscode.commands.executeCommand('vscode.open', uri, { preview: false }));
        if (group || group === '')
            ConfigurationsHandler.createProcTemplate(workspace, group).then((uri) => vscode.commands.executeCommand('vscode.open', uri, { preview: false }));
    }

    public static async provideCodeLenses(document: vscode.TextDocument): Promise<vscode.CodeLens[]> {
        if (document.isClosed) return [];
        const workspace = vscode.workspace.getWorkspaceFolder(document.uri);
        if (!workspace) return [];
        const documentRelativeUri = vscode.workspace.asRelativePath(document.uri);

        const configNodes = await retrieveConfigurationNodes(workspace.uri, document.uri);

        return generateConfigurationFilesCodeActions(
            !configNodes.procGrps.exists,
            !(configNodes.pgmConf.exists || configNodes.bridgeJson.exists || configNodes.ebgFolder.exists),
            configNodes,
            workspace.uri,
            documentRelativeUri)
            .map(x => new vscode.CodeLens(document.lineAt(0).range, x.command));
    }
}
