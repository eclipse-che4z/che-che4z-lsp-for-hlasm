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

export function getWorkspacePath(): string {
    return vscode.workspace.workspaceFolders[0].uri.fsPath;;
}

export function get_editor(workspace_file: string): vscode.TextEditor {
    const editor = vscode.window.activeTextEditor;
    assert.equal(editor.document.uri.fsPath, path.join(getWorkspacePath(), workspace_file));

    return editor;
}

export async function showDocument(workspace_file: string) {
    const files = await vscode.workspace.findFiles(workspace_file);

    assert.ok(files && files[0]);
    const file = files[0];

    // open and show the file
    const document = await vscode.workspace.openTextDocument(file);

    await vscode.window.showTextDocument(document);
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

export function sleep(ms: number): Promise<unknown> {
    return new Promise((resolve) => { setTimeout(resolve, ms); });
}

