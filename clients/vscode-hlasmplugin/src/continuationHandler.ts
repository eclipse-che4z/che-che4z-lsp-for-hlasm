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

    private extractContinuationSymbolSingleLine(document: vscode.TextDocument, line: number, continuationOffset: number): string {
        if (line >= 0 && line < document.lineCount) {
            const lineText = document.lineAt(line).text;
            if (continuationOffset < lineText.length) {
                const character = lineText.at(continuationOffset);
                if (character != " ")
                    return character;
            }
        }
        return "";
    }

    private extractMostCommonContinuationSymbol(document: vscode.TextDocument, continuationOffset: number): string {
        let continuationSymbols: { [name: string]: number } = {};
        for (let i = 0; i < Math.min(document.lineCount, 5000); ++i) {
            const line = document.lineAt(i).text;
            if (line.length <= continuationOffset)
                continue;
            const candidate = line.at(continuationOffset);
            if (candidate == ' ')
                continue;
            continuationSymbols[candidate] = (continuationSymbols[candidate] || 0) + 1;
        }
        return Object.entries(continuationSymbols).reduce((prev, current) => prev[1] >= current[1] ? prev : current, ["X", 0])[0];
    }

    private extractContinuationSymbol(document: vscode.TextDocument, line: number, continuationOffset: number): string {
        return this.extractContinuationSymbolSingleLine(document, line, continuationOffset) ||
            this.extractContinuationSymbolSingleLine(document, line - 1, continuationOffset) ||
            this.extractContinuationSymbolSingleLine(document, line + 1, continuationOffset) ||
            this.extractMostCommonContinuationSymbol(document, continuationOffset);
    }

    // insert continuation character X to the current line
    insertContinuation(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number, continueColumn: number) {
        const sel = editor.selection;

        // retrieve continuation information
        const line = sel.active.line;
        const col = sel.active.character;
        const isContinued = isLineContinued(editor.document, line, continuationOffset);
        const doc = editor.document;
        const lineText = doc.lineAt(line).text;
        const eol = doc.eol == vscode.EndOfLine.LF ? '\n' : '\r\n';

        let contSymbol = this.extractContinuationSymbol(doc, line, continuationOffset);
        if (!isContinued) {
            let initialLineSize = editor.document.lineAt(line).text.length;
            if (initialLineSize <= continuationOffset) {
                edit.insert(new vscode.Position(line, col),
                    ' '.repeat(continuationOffset - col) + contSymbol + eol +
                    ' '.repeat(continueColumn));
            }
            else {
                let reinsert = '';
                let ignoredPart = '';
                if (col < continuationOffset)
                    reinsert = lineText.substring(col, continuationOffset).trimEnd();

                ignoredPart = lineText.substring(continuationOffset + 1);
                // see https://github.com/microsoft/vscode/issues/32058 why replace does not work
                const insertionPoint = new vscode.Position(line, Math.min(col, continuationOffset));
                edit.delete(new vscode.Range(insertionPoint, new vscode.Position(line, lineText.length)));
                edit.insert(insertionPoint,
                    ' '.repeat(Math.max(continuationOffset - col, 0)) + contSymbol + ignoredPart + eol +
                    ' '.repeat(continueColumn) + reinsert);
            }
        }
        // add extra continuation on already continued line
        else {
            let prefix = '';
            let reinsert = '';
            if (col < continuationOffset) {
                reinsert = lineText.substring(col, continuationOffset).trimEnd();
                prefix = ' '.repeat(continuationOffset - col) + lineText.substring(continuationOffset, lineText.length);
            }
            else {
                prefix = lineText.substring(col, lineText.length);
            }
            const insertionPoint = new vscode.Position(line, col);
            edit.delete(new vscode.Range(insertionPoint, new vscode.Position(line, lineText.length)));
            edit.insert(insertionPoint,
                prefix + eol +
                ' '.repeat(continueColumn) + reinsert + ' '.repeat(continuationOffset - continueColumn - reinsert.length) + contSymbol);
            setImmediate(() => setCursor(editor, new vscode.Position(line + 1, continueColumn + reinsert.length)));
        }
    }

    // remove continuation from previous line
    removeContinuation(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number) {
        const isThisContinued = isLineContinued(editor.document, editor.selection.active.line, continuationOffset);
        const isPrevContinued = isLineContinued(editor.document, editor.selection.active.line - 1, continuationOffset);
        const line = editor.selection.active.line;
        const col = editor.selection.active.character;

        if (editor.selection.active.line > 0 && isPrevContinued) {
            edit.delete(
                new vscode.Range(
                    new vscode.Position(line - 1, editor.document.lineAt(line - 1).text.length),
                    new vscode.Position(line, editor.document.lineAt(line).text.length)));
            if (!isThisContinued) {
                const continuationPosition = new vscode.Position(
                    editor.selection.active.line - 1, continuationOffset);
                edit.replace(
                    new vscode.Range(
                        new vscode.Position(editor.selection.active.line - 1, continuationOffset),
                        new vscode.Position(editor.selection.active.line - 1, continuationOffset + 1)
                    ),
                    ' '
                );
            }
            setImmediate(() => setCursor(editor,
                new vscode.Position(
                    line - 1,
                    editor.document.lineAt(line - 1).text.substring(0, continuationOffset).trimEnd().length
                )
            ));
        }
    }
}
