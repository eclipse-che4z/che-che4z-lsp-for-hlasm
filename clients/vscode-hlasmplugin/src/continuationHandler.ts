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

import { HLASMSemanticHighlightingFeature } from './hlasmSemanticHighlighting'
import { setCursor } from './customEditorCommands'
import { getConfig } from './eventsHandler'

/**
 * Implements removeContinuation and insertContinuation commands.
 */
export class ContinuationHandler {
    private highlight: HLASMSemanticHighlightingFeature;

    /**
     * @param highlight Needed for its continuation info
     */
    constructor(highlight: HLASMSemanticHighlightingFeature) {
        this.highlight = highlight;
    }

    dispose() {
        this.highlight.dispose();
    }

    // insert continuation character X to the current line
    insertContinuation(editor: vscode.TextEditor, edit: vscode.TextEditorEdit) {
        if (!getConfig<boolean>('continuationHandling', false))
            return;

        const continuationOffset = this.highlight.getContinuation(-1, editor.document.uri.toString());
        const continueColumn = this.highlight.getContinueColumn(editor.document.uri.toString());

        // not a continued line, create new continuation
        if (this.highlight.getContinuation(editor.selection.active.line, editor.document.uri.toString()) == -1) {
            var lastChar = editor.selection.active.character;
            if (editor.selection.active.character < continuationOffset) {
                lastChar = editor.document.lineAt(editor.selection.active).text.length;
                lastChar = (lastChar < continuationOffset) ? lastChar : continuationOffset;
                edit.insert(
                    new vscode.Position(editor.selection.active.line, lastChar), " ".repeat(continuationOffset - lastChar));
            }
            const continuationPosition = new vscode.Position(editor.selection.active.line, continuationOffset);
            edit.replace(
                new vscode.Range(
                    continuationPosition,
                    new vscode.Position(editor.selection.active.line, continuationOffset + 1)),
                "X");

            edit.insert(
                new vscode.Position(
                    editor.selection.active.line,
                    editor.document.lineAt(editor.selection.active).text.length),
                "\r\n".concat(" ".repeat(continueColumn)));
        }
        // add extra continuation on already continued line
        else
            edit.insert(new vscode.Position(
                editor.selection.active.line,
                editor.document.lineAt(editor.selection.active).text.length),
                "\r\n".concat(" ".repeat(continuationOffset).concat("X")));
    }

    // remove continuation from previous line
    removeContinuation(editor: vscode.TextEditor, edit: vscode.TextEditorEdit) {
        if (!getConfig<boolean>('continuationHandling', false))
            return;

        const continuationOffset = this.highlight.getContinuation(-1, editor.document.uri.toString());

        if (editor.selection.active.line > 0 &&
            this.highlight.getContinuation(editor.selection.active.line - 1, editor.document.uri.toString()) != -1) {
            const continuationPosition = new vscode.Position(
                editor.selection.active.line - 1, continuationOffset);
            edit.delete(
                new vscode.Range(
                    new vscode.Position(editor.selection.active.line, editor.document.lineAt(editor.selection.active).text.length),
                    new vscode.Position(
                        editor.selection.active.line - 1,
                        editor.document.lineAt(editor.selection.active.line - 1).text.length)));

            edit.replace(new vscode.Range(
                continuationPosition,
                new vscode.Position(continuationPosition.line, continuationOffset + 1)),
                " ");

            setCursor(editor, continuationPosition);
        }
    }
}