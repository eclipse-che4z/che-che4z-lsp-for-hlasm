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
import { stripJsonComments, textDecode } from './tools.common';

async function readJsonFile(uri: vscode.Uri): Promise<unknown> {
    try {
        return JSON.parse(stripJsonComments(textDecode(await vscode.workspace.fs.readFile(uri))));
    }
    catch (_) {
        return undefined;
    }
}

function isArray(o: unknown): o is unknown[] { return Array.isArray(o); }

function isArrayOf<T>(o: unknown, pred: (o: unknown) => o is T): o is T[];
function isArrayOf<A, T extends A>(o: A[], pred: (o: T) => o is T): o is T[];
function isArrayOf(o: unknown, pred: (o: unknown) => boolean) {
    if (!isArray(o)) return false;
    return !o.some(x => !pred(x));
}

function isString(o: unknown): o is string { return typeof o === 'string'; }

function isObject(o: unknown): o is object { return o !== null && typeof o === 'object'; }

function hasStringProgram(o: object): o is { program: string } {
    return 'program' in o && typeof o.program === 'string';
}

function isPgmConfLike(o: unknown): o is {
    pgms?: { program: string }[],
    alwaysRecognize?: string[],
} {
    if (!isObject(o)) return false;
    if ('pgms' in o) {
        if (!isArrayOf(o.pgms, isObject)) return false;
        if (!isArrayOf(o.pgms, hasStringProgram)) return false;
    }
    if ('alwaysRecognize' in o) {
        if (!isArrayOf(o.alwaysRecognize, isString)) return false;
    }
    return true;
}

function isProcGrpsLike(o: unknown): o is {
    pgroups?: {
        libs?: (string | { path: string } | (object & { path: never }))[];
    }[];
} {
    if (!isObject(o)) return false;

    if ('pgroups' in o) {
        if (!isArrayOf(o.pgroups, isObject)) return false;
        for (const x of o.pgroups) {
            if (!('libs' in x))
                continue;
            if (!isArray(x.libs)) return false;
            for (const l of x.libs) {
                if (typeof l === 'string') continue;
                if (!isObject(l)) return false;
                if ('path' in l && typeof l.path !== 'string') return false;
            }
        }
    }
    return true;
}

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
    async generateWildcards(workspaceUri: vscode.Uri, reloadedFile?: vscode.Uri): Promise<RegExp[] | undefined> {
        const procGrps = vscode.Uri.joinPath(workspaceUri, hlasmplugin_folder, proc_grps_file);
        const pgmConf = vscode.Uri.joinPath(workspaceUri, hlasmplugin_folder, pgm_conf_file);
        // the reloaded file is not a config file
        if (reloadedFile && reloadedFile != pgmConf && reloadedFile != procGrps)
            return undefined;
        // configs do not exist
        if (!await ConfigurationsHandler.configFilesExist(workspaceUri))
            return [];

        //clear expressions
        const definedExpressions: RegExp[] = [];
        // get user-defined wildcards
        const pgmContent = await readJsonFile(pgmConf);
        const procContent = await readJsonFile(procGrps);

        if (!isPgmConfLike(pgmContent)) return undefined;
        if (!isProcGrpsLike(procContent)) return undefined;

        // convert each pgm to regex
        if (pgmContent.pgms) {
            (pgmContent.pgms as any[]).forEach(pgm => {
                const regex = this.convertWildcardToRegex(vscode.Uri.joinPath(workspaceUri, pgm.program as string).toString());
                if (regex)
                    definedExpressions.push(regex);
            });
        }

        // convert each wildcard to regex
        if (pgmContent.alwaysRecognize) {
            pgmContent.alwaysRecognize.forEach(strExpr => {
                const regex = this.convertWildcardToRegex(vscode.Uri.joinPath(workspaceUri, strExpr).toString());
                if (regex)
                    definedExpressions.push(regex);
            });
        }

        // convert each pgroup library path to regex
        if (procContent.pgroups) {
            procContent.pgroups.forEach(pgroup => {
                if (pgroup.libs)
                    pgroup.libs.forEach(lib => {
                        if (typeof lib !== 'string' && !('path' in lib)) return;
                        const l = typeof lib === 'string' ? lib : lib.path;
                        const regex = this.convertWildcardToRegex(vscode.Uri.joinPath(workspaceUri, l, '*').toString());
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
            {
                nodes: configNodes,
                ws: workspace.uri,
                documentRelativeUri
            })
            .map(x => new vscode.CodeLens(document.lineAt(0).range, x.command));
    }
}
