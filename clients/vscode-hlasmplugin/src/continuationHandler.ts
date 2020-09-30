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

import { setCursor, isLineContinued } from './customEditorCommands'
import { getConfig } from './eventsHandler'

/**
 * Implements removeContinuation and insertContinuation commands.
 */
export class ContinuationHandler {
    dispose() { }

    // insert continuation character X to the current line
    insertContinuation(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number, continueColumn: number) {
        if (!getConfig<boolean>('continuationHandling', false))
            return;

        // retrieve continuation information
        const isContinued = isLineContinued(editor.document, editor.selection.active.line, continuationOffset);

        if (!isContinued) {
            if (editor.selection.active.character < continuationOffset) {
                const lastChar =
                    (editor.selection.active.character < continuationOffset)
                        ? editor.document.lineAt(editor.selection.active).text.length
                        : continuationOffset;

                edit.insert(
                    new vscode.Position(editor.selection.active.line, lastChar),
                    " ".repeat(continuationOffset - lastChar));
            }
            const continuationPosition = new vscode.Position(editor.selection.active.line, continuationOffset);
            edit.replace(
                new vscode.Range(
                    continuationPosition,
                    new vscode.Position(editor.selection.active.line, continuationOffset + 1)),
                'X\r\n' + ' '.repeat(continueColumn));
        }
        // add extra continuation on already continued line
        else
            edit.insert(new vscode.Position(
                editor.selection.active.line,
                editor.document.lineAt(editor.selection.active).text.length),
                "\r\n" + " ".repeat(continuationOffset) + "X");
    }

    // remove continuation from previous line
    removeContinuation(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number) {
        if (!getConfig<boolean>('continuationHandling', false))
            return;

        const wasLastContinued = isLineContinued(editor.document, editor.selection.active.line - 1, continuationOffset);

        if (editor.selection.active.line > 0 && wasLastContinued) {
            const continuationPosition = new vscode.Position(
                editor.selection.active.line - 1, continuationOffset);
            edit.delete(
                new vscode.Range(
                    new vscode.Position(editor.selection.active.line, editor.document.lineAt(editor.selection.active).text.length),
                    new vscode.Position(editor.selection.active.line - 1, continuationOffset)));

            setCursor(editor, continuationPosition);
        }
    }
}
