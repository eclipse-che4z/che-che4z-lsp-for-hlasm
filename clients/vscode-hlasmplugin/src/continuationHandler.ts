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
import { LinePositionsInfo, setCursor } from './customEditorCommands'
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

        const info = new LinePositionsInfo(
            editor.selection.active,
            this.highlight.getContinuation(-1, editor.document.uri.toString())
        );

        const continueColumn = this.highlight.getContinueColumn(editor.document.uri.toString());
        // not a continued line, create new continuation
        if (this.highlight.getContinuation(info.currentPosition.line, editor.document.uri.toString()) == -1) {
            var lastChar = info.currentPosition.character;
            if (info.currentPosition.character < info.continuationOffset) {
                lastChar = editor.document.lineAt(info.currentPosition).text.length;
                lastChar = (lastChar < info.continuationOffset) ? lastChar : info.continuationOffset;
                edit.insert(
                    new vscode.Position(info.currentPosition.line, lastChar), " ".repeat(info.continuationOffset - lastChar));
            }
            const continuationPosition = new vscode.Position(info.currentPosition.line, info.continuationOffset);
            edit.replace(
                new vscode.Range(
                    continuationPosition,
                    new vscode.Position(info.currentPosition.line, info.continuationOffset + 1)),
                "X");

            edit.insert(
                new vscode.Position(
                    info.currentPosition.line,
                    editor.document.lineAt(info.currentPosition).text.length),
                "\r\n".concat(" ".repeat(continueColumn)));
        }
        // add extra continuation on already continued line
        else
            edit.insert(new vscode.Position(
                info.currentPosition.line,
                editor.document.lineAt(info.currentPosition).text.length),
                "\r\n".concat(" ".repeat(info.continuationOffset).concat("X")));
    }

    // remove continuation from previous line
    removeContinuation(editor: vscode.TextEditor, edit: vscode.TextEditorEdit) {
        if (!getConfig<boolean>('continuationHandling', false))
            return;

        const info = new LinePositionsInfo(
            editor.selection.active,
            this.highlight.getContinuation(-1, editor.document.uri.toString())
        );

        if (info.currentPosition.line > 0 &&
            this.highlight.getContinuation(info.currentPosition.line - 1, editor.document.uri.toString()) != -1) {
            const continuationPosition = new vscode.Position(
                info.currentPosition.line - 1, info.continuationOffset);
            edit.delete(
                new vscode.Range(
                    new vscode.Position(info.currentPosition.line, editor.document.lineAt(info.currentPosition).text.length),
                    new vscode.Position(
                        info.currentPosition.line - 1,
                        editor.document.lineAt(info.currentPosition.line - 1).text.length)));

            edit.replace(new vscode.Range(
                continuationPosition,
                new vscode.Position(continuationPosition.line, info.continuationOffset + 1)),
                " ");

            setCursor(editor, continuationPosition);
        }
    }
}