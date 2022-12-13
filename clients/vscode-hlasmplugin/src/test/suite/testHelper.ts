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
import * as assert from 'assert';
import * as vscode from "vscode";
import * as path from 'path';
import { integer } from 'vscode-languageclient';

export function getWorkspacePath(): string {
    return vscode.workspace.workspaceFolders[0].uri.fsPath;
}

export async function showDocument(workspace_file: string, language_id: string | undefined = undefined) {
    const files = await vscode.workspace.findFiles(workspace_file);

    assert.ok(files && files[0]);
    const file = files[0];

    // open and show the file
    let document = await vscode.workspace.openTextDocument(file);
    if (language_id)
        document = await vscode.languages.setTextDocumentLanguage(document, language_id);

    return { editor: await vscode.window.showTextDocument(document), document };
}

export async function closeAllEditors() {
    while (vscode.window.activeTextEditor !== undefined) {
        await vscode.commands.executeCommand('workbench.action.closeActiveEditor');
        await sleep(500);
    }
}

export function moveCursor(editor: vscode.TextEditor, position: vscode.Position) {
    editor.selection = new vscode.Selection(position, position);
}

export async function toggleBreakpoints(file: string, lines: Array<integer>) {
    const editor = (await showDocument(file)).editor;
    await sleep(1000);

    for (const line of lines) {
        moveCursor(editor, new vscode.Position(line, 0));
        await vscode.commands.executeCommand('editor.debug.action.toggleBreakpoint');
    }

    await sleep(1000);
}

export async function removeAllBreakpoints() {

    await vscode.commands.executeCommand('workbench.debug.viewlet.action.removeAllBreakpoints');
    await sleep(1000);
}

export async function debugStartSession(): Promise<vscode.DebugSession> {
    const session_started_event = new Promise<vscode.DebugSession>((resolve) => {
        // when the debug session starts
        const disposable = vscode.debug.onDidStartDebugSession((session) => {
            disposable.dispose();
            resolve(session);
        });
    });
    // start debugging
    if (!await vscode.debug.startDebugging(vscode.workspace.workspaceFolders[0], 'Macro tracer: current program'))
        throw new Error("Failed to start a debugging session");

    const session = await session_started_event;
    await sleep(1000);

    return session;
}

export async function debugContinue() {
    await vscode.commands.executeCommand('workbench.action.debug.continue');
    await sleep(1000);
}

export async function debugStepOver(steps: integer) {
    while (steps) {
        await vscode.commands.executeCommand('workbench.action.debug.stepOver');
        await sleep(1000);
        steps--;
    }
}

export async function debugStepInto() {
    await vscode.commands.executeCommand('workbench.action.debug.stepInto');
    await sleep(1000);
}

export async function debugStop() {
    await vscode.commands.executeCommand('workbench.action.debug.stop');
}

export async function insertString(editor: vscode.TextEditor, position: vscode.Position, str: string): Promise<vscode.Position> {
    await editor.edit(edit => {
        edit.insert(position, str);
    });

    // Get number of lines in string and compute the new end position
    const str_split = str.split('\n');
    const lines = str_split.length;

    const movePosition = new vscode.Position(position.line + lines - 1, lines == 1 ? position.character + str.length : str_split[lines].length);
    editor.selection = new vscode.Selection(movePosition, movePosition);

    return movePosition;
}

export function sleep(ms: number): Promise<void> {
    return new Promise<void>((resolve) => { setTimeout(resolve, ms); });
}

export function timeout(ms: number, error_message: string | undefined = undefined): Promise<void> {
    return new Promise<void>((_, reject) => { setTimeout(() => reject(error_message && Error(error_message)), ms); });
}

export async function waitForDiagnostics() {
    return new Promise<[vscode.Uri, vscode.Diagnostic[]][]>((resolve, reject) => {
        const listener = vscode.languages.onDidChangeDiagnostics((_) => {
            listener.dispose();
            resolve(vscode.languages.getDiagnostics());
        });
    });
}
