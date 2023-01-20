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
import * as vscode from 'vscode';

const debuggerWaitRequests = new Map<string, () => void>();

export function registerWaitRequest(message: string, sessionId: string): Promise<void> {
    return new Promise<void>((resolve) => debuggerWaitRequests.set(sessionId + '_' + message, resolve));
}

export function popWaitRequestResolver(message: string, sessionId: string): () => void {
    const key = sessionId + '_' + message;

    const result = debuggerWaitRequests.get(key);
    if (result)
        debuggerWaitRequests.delete(key);

    return result;
}

export function activeEditorChanged(): Promise<vscode.TextEditor> {
    return new Promise<vscode.TextEditor>((resolve) => {
        const listener = vscode.window.onDidChangeActiveTextEditor((e) => {
            if (e) {
                listener.dispose();
                resolve(e);
            }
        })
    });
}

export function getWorkspacePath(): string {
    return vscode.workspace.workspaceFolders[0].uri.fsPath;
}

export async function getWorkspaceFile(workspace_file: string) {
    const files = await vscode.workspace.findFiles(workspace_file);

    assert.ok(files && files[0]);

    return files[0];
}

export async function showDocument(workspace_file: string, language_id: string | undefined = undefined) {
    // open and show the file
    let document = await vscode.workspace.openTextDocument(await getWorkspaceFile(workspace_file));
    if (language_id)
        document = await vscode.languages.setTextDocumentLanguage(document, language_id);

    const visible = activeEditorChanged();
    const result = { editor: await vscode.window.showTextDocument(document, { preview: false }), document };
    assert.strictEqual(await visible, result.editor);
    return result;
}

export async function closeAllEditors() {
    await vscode.commands.executeCommand('workbench.action.files.revert');
    // workbench.action.closeAllEditors et al. saves content
    await vscode.commands.executeCommand('workbench.action.closeAllEditors');
}

export async function addBreakpoints(file: string, lines: Array<number>) {
    const document = (await showDocument(file, 'hlasm')).document;

    await vscode.debug.addBreakpoints(lines.map(l => new vscode.SourceBreakpoint(new vscode.Location(document.uri, new vscode.Position(l, 0)), true)));
}

export async function removeAllBreakpoints() {
    await vscode.debug.removeBreakpoints(vscode.debug.breakpoints);
}

function sessionStoppedEvent(session: vscode.DebugSession = vscode.debug.activeDebugSession) {
    if (!session)
        return Promise.resolve();
    else
        return registerWaitRequest("scopes", session.id);
}

export async function debugStartSession(waitForStopped = true): Promise<vscode.DebugSession> {
    const session_started_event = new Promise<vscode.DebugSession>((resolve) => {
        // when the debug session starts
        const disposable = vscode.debug.onDidStartDebugSession((session) => {
            disposable.dispose();
            if (waitForStopped)
                sessionStoppedEvent(session).then(() => resolve(session));
            else
                resolve(session);
        });
    });
    // start debugging
    if (!await vscode.debug.startDebugging(vscode.workspace.workspaceFolders[0], 'Macro tracer: current program'))
        throw new Error("Failed to start a debugging session");

    const session = await session_started_event;

    return session;
}

export async function debugContinue() {
    const ready = sessionStoppedEvent();
    await vscode.commands.executeCommand('workbench.action.debug.continue');
    await ready;
}

export async function debugStepOver(steps: number) {
    while (steps) {
        const ready = sessionStoppedEvent();
        await vscode.commands.executeCommand('workbench.action.debug.stepOver');
        await ready;
        steps--;
    }
}

export async function debugStepInto() {
    const ready = sessionStoppedEvent();
    await vscode.commands.executeCommand('workbench.action.debug.stepInto');
    await ready;
}

export async function debugStop() {
    await vscode.debug.stopDebugging();
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

export async function waitForDiagnostics(file: string | vscode.Uri, nonEmptyOnly: boolean = false) {
    const result = new Promise<vscode.Diagnostic[]>((resolve) => {
        const file_promise = typeof file === 'string' ? getWorkspaceFile(file).then(uri => uri.toString()) : Promise.resolve(file.toString());

        let listener = vscode.languages.onDidChangeDiagnostics((e) => {
            file_promise.then((file) => {
                if (!listener)
                    return;
                const forFile = e.uris.find(v => v.toString() === file);
                if (!forFile)
                    return;
                const diags = vscode.languages.getDiagnostics(forFile);
                if (nonEmptyOnly && diags.length === 0)
                    return;
                listener.dispose();
                listener = null;
                resolve(diags);
            });
        });
    });

    return result;
}
