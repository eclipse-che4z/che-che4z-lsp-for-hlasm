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
                const character = lineText.at(continuationOffset)!;
                if (character != " ")
                    return character;
            }
        }
        return "";
    }

    private extractMostCommonContinuationSymbol(document: vscode.TextDocument, continuationOffset: number): string {
        let continuationSymbols: { [name: string]: number } = {};
        for (let i = 0; i < Math.min(document.lineCount, 5000); ++i) {
            const lineText = document.lineAt(i).text;
            if (continuationOffset >= lineText.length)
                continue;
            const candidate = lineText.at(continuationOffset)!;
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
        const doc = editor.document;
        const eol = doc.eol == vscode.EndOfLine.LF ? '\n' : '\r\n';

        const selections: { [line: number]: vscode.Selection[] } = editor.selections
            .filter(s => s.isSingleLine)
            .map(s => s.start.character <= continuationOffset ? s : new vscode.Selection(new vscode.Position(s.start.line, continuationOffset), new vscode.Position(s.start.line, continuationOffset)))
            .map((s) => ({ line: s.start.line, selection: s.end.character <= continuationOffset ? s : new vscode.Selection(s.start, new vscode.Position(s.end.line, continuationOffset)) }))
            .reduce((r, v): { [line: number]: vscode.Selection[] } => { r[v.line] = r[v.line] || []; r[v.line].push(v.selection); return r; }, Object.create({}));

        let new_selection: vscode.Selection[] = [];
        let idx = Object.entries(selections).length;
        for (const line_sel of Object.entries(selections).sort((a, b) => +b[0] - +a[0])) {
            // retrieve continuation information
            const line = +line_sel[0];
            const lineText = doc.lineAt(line).text;
            const sel = ((x: vscode.Selection[]): vscode.Selection[] => {
                if (x.length == 1 && x[0].isEmpty)
                    return [new vscode.Selection(x[0].start, new vscode.Position(line, Math.min(continuationOffset, lineText.length)))];
                return x.sort((l, r) => l.start.character - r.start.character);
            })(line_sel[1]);

            let reinsert = '';
            for (let s of sel)
                reinsert += lineText.substring(s.start.character, s.end.character);
            const trimmed_reinsert = ' '.repeat(continueColumn) + reinsert.trimEnd();
            const contSymbol = this.extractContinuationSymbol(doc, line, continuationOffset);

            if (isLineContinued(editor.document, line, continuationOffset)) {
                edit.insert(new vscode.Position(line, continuationOffset), ' '.repeat(reinsert.length));
                edit.insert(new vscode.Position(line + 1, 0), trimmed_reinsert.padEnd(continuationOffset) + contSymbol + eol);
            }
            else {
                if (lineText.length < continuationOffset)
                    edit.insert(new vscode.Position(line, lineText.length), ' '.repeat(continuationOffset - lineText.length + reinsert.length) + contSymbol + eol + trimmed_reinsert);
                else {
                    edit.insert(new vscode.Position(line, continuationOffset), ' '.repeat(reinsert.length));
                    edit.replace(new vscode.Selection(new vscode.Position(line, continuationOffset), new vscode.Position(line, continuationOffset + 1)), contSymbol);
                    if (line == doc.lineCount - 1) // missing newline
                        edit.insert(new vscode.Position(line, lineText.length), eol);
                    edit.insert(new vscode.Position(line + 1, 0), trimmed_reinsert + eol);
                }
            }
            for (let s of sel.sort((l, r) => r.start.character - l.start.character))
                edit.delete(s);
            new_selection.push(new vscode.Selection(new vscode.Position(line + idx, trimmed_reinsert.length), new vscode.Position(line + idx, trimmed_reinsert.length)));
            idx--;
        }
        setImmediate(() => { editor.selections = new_selection; });
    }

    private extractLineRangesForRemoval(editor: vscode.TextEditor, continuationOffset: number): { start: number, end: number }[] {
        const selection_to_lines = (x: vscode.Selection) => {
            let result = [];
            for (let l = x.start.line; l <= x.end.line; ++l)
                result.push(l);
            return result;
        };
        const all_lines = [... new Set(editor.selections.map(selection_to_lines).flat(1))].sort((l, r) => l - r);
        let last = -2;
        let result: { start: number, end: number }[] = [];
        for (let l of all_lines) {
            if (l != last + 1) {
                if (l > 0 && isLineContinued(editor.document, l - 1, continuationOffset)) {
                    result.push({ start: l, end: l });
                    last = l;
                }
            }
            else {
                result[result.length - 1].end = l;
                last = l;
            }
        }
        return result;
    }

    // remove continuation from previous line
    removeContinuation(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number) {
        let new_selection: vscode.Selection[] = [];
        for (const line_range of this.extractLineRangesForRemoval(editor, continuationOffset).sort((l, r) => { return l.start - r.start; })) {
            edit.delete(
                new vscode.Range(
                    new vscode.Position(line_range.start - 1, editor.document.lineAt(line_range.start - 1).text.length),
                    new vscode.Position(line_range.end, editor.document.lineAt(line_range.end).text.length)));
            if (!isLineContinued(editor.document, line_range.end, continuationOffset)) {
                const continuationPosition = new vscode.Position(line_range.start - 1, continuationOffset);
                edit.replace(
                    new vscode.Range(
                        new vscode.Position(line_range.start - 1, continuationOffset),
                        new vscode.Position(line_range.start - 1, continuationOffset + 1)
                    ),
                    ' '
                );
            }
            let new_cursor = new vscode.Position(
                line_range.start - 1,
                editor.document.lineAt(line_range.start - 1).text.substring(0, continuationOffset).trimEnd().length
            );
            new_selection.push(new vscode.Selection(new_cursor, new_cursor));
        }
        editor.selections = new_selection;
    }

    rearrangeSequenceNumbers(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number) {
        const selections: { [line: number]: vscode.Selection[] } = editor.selections
            .filter(s => s.isSingleLine)
            .map((s) => ({ line: s.start.line, selection: s }))
            .reduce((r, v): { [line: number]: vscode.Selection[] } => { r[v.line] = r[v.line] || []; r[v.line].push(v.selection); return r; }, Object.create({}));
        for (const line_sel of Object.entries(selections).sort((a, b) => +b[0] - +a[0])) {
            const sel = line_sel[1];

            // retrieve continuation information
            const line = +line_sel[0];
            const doc = editor.document;
            const lineText = doc.lineAt(line).text;
            const selectionLength = sel.reduce((r, v) => r + (v.end.character - v.start.character), 0);

            if (lineText.length > continuationOffset) {
                const notSpace = Math.max(lineText.lastIndexOf(' '), continuationOffset) + 1;
                const deletionStart = continuationOffset + (notSpace + 1 == lineText.length || lineText.length - notSpace > 8 ? 0 : 1);
                if (notSpace - deletionStart < selectionLength) {
                    edit.insert(new vscode.Position(line, continuationOffset), ' '.repeat(selectionLength - (notSpace - deletionStart)));
                }
                else if (notSpace - deletionStart > selectionLength) {
                    // the end of line segment is either a single character, or longer than 8 => assume continuation symbol is present
                    // otherwise assume only sequence symbols are present
                    if (lineText.substring(deletionStart + selectionLength, notSpace).trim().length == 0) // only delete spaces!!!
                        edit.delete(
                            new vscode.Range(
                                new vscode.Position(line, deletionStart + selectionLength),
                                new vscode.Position(line, notSpace)
                            )
                        );
                }
            }
            for (let s of sel.sort((l, r) => r.start.character - l.start.character))
                if (!s.isEmpty)
                    edit.delete(s);
        }
    }
}
