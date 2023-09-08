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

import { codepointToOffset, isLineContinued, offsetToCodepoint } from './customEditorCommands'

/**
 * Implements removeContinuation and insertContinuation commands.
 */

function extractContinuationSymbolSingleLine(document: vscode.TextDocument, line: number, continuationOffset: number): string {
    if (line >= 0 && line < document.lineCount) {
        const lineText = document.lineAt(line).text;
        if (continuationOffset < lineText.length) {
            const character = [...lineText][continuationOffset] ?? ' ';
            if (character !== ' ')
                return character;
        }
    }
    return '';
}

function extractMostCommonContinuationSymbol(document: vscode.TextDocument, continuationOffset: number): string {
    let continuationSymbols: { [name: string]: number } = {};
    for (let i = 0; i < Math.min(document.lineCount, 5000); ++i) {
        const lineText = document.lineAt(i).text;
        if (continuationOffset >= lineText.length)
            continue;
        const candidate = [...lineText][continuationOffset] ?? ' ';
        if (candidate === ' ')
            continue;
        continuationSymbols[candidate] = (continuationSymbols[candidate] || 0) + 1;
    }
    return Object.entries(continuationSymbols).reduce((prev, current) => prev[1] >= current[1] ? prev : current, ["X", 0])[0];
}

function extractContinuationSymbol(document: vscode.TextDocument, line: number, continuationOffset: number): string {
    return extractContinuationSymbolSingleLine(document, line, continuationOffset) ||
        extractContinuationSymbolSingleLine(document, line - 1, continuationOffset) ||
        extractContinuationSymbolSingleLine(document, line + 1, continuationOffset) ||
        extractMostCommonContinuationSymbol(document, continuationOffset);
}

function clampSelection(doc: vscode.TextDocument, selection: vscode.Selection, offset: number) {
    const text = doc.lineAt(selection.start.line).text;
    const s = offsetToCodepoint(text, selection.start.character)!;
    const e = offsetToCodepoint(text, selection.end.character)!;

    return new vscode.Selection(
        new vscode.Position(selection.start.line, s <= offset ? selection.start.character : codepointToOffset(text, offset)!),
        new vscode.Position(selection.start.line, e <= offset ? selection.end.character : codepointToOffset(text, offset)!)
    );
}

function codepointPad(s: string, l: number, c: string = ' ') {
    const cl = offsetToCodepoint(s, s.length)!;
    if (cl >= l) return s;
    return s + c.repeat(l - cl);
}

// insert continuation character X to the current line
export function insertContinuation(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationColumn: number, initialBlanks: number) {
    const doc = editor.document;
    const eol = doc.eol == vscode.EndOfLine.LF ? '\n' : '\r\n';

    const selections: { [line: number]: vscode.Selection[] } = editor.selections
        .filter(s => s.isSingleLine && s.start.line < doc.lineCount)
        .map((s) => ({ line: s.start.line, selection: clampSelection(doc, s, continuationColumn) }))
        .reduce((r, v): { [line: number]: vscode.Selection[] } => { r[v.line] = r[v.line] || []; r[v.line].push(v.selection); return r; }, Object.create({}));

    let new_selection: vscode.Selection[] = [];
    let idx = Object.entries(selections).length;
    for (const line_sel of Object.entries(selections).sort((a, b) => +b[0] - +a[0])) {
        // retrieve continuation information
        const line = +line_sel[0];
        const lineText = doc.lineAt(line).text;
        const sel = ((x: vscode.Selection[]): vscode.Selection[] => {
            if (x.length == 1 && x[0].isEmpty)
                return [new vscode.Selection(x[0].start, new vscode.Position(line, Math.min(codepointToOffset(lineText, continuationColumn) ?? lineText.length, lineText.length)))];
            return x.sort((l, r) => l.start.character - r.start.character);
        })(line_sel[1]);

        let reinsert = '';
        for (let s of sel)
            reinsert += lineText.substring(s.start.character, s.end.character);
        const trimmed_reinsert = ' '.repeat(initialBlanks) + reinsert.trimEnd();
        const contSymbol = extractContinuationSymbol(doc, line, continuationColumn);

        const cont = isLineContinued(editor.document, line, continuationColumn);
        if (cont) {
            edit.insert(new vscode.Position(line, cont.actualOffset), ' '.repeat(offsetToCodepoint(reinsert, reinsert.length)!));
            edit.insert(new vscode.Position(line + 1, 0), codepointPad(trimmed_reinsert, continuationColumn) + contSymbol + eol);
        }
        else {
            const codePointLen = offsetToCodepoint(lineText, lineText.length)!;
            const reinsertCodePointLen = offsetToCodepoint(reinsert, reinsert.length)!;
            if (codePointLen < continuationColumn)
                edit.insert(new vscode.Position(line, lineText.length), ' '.repeat(continuationColumn - codePointLen + reinsertCodePointLen) + contSymbol + eol + trimmed_reinsert);
            else {
                const continuationOffset = codepointToOffset(lineText, continuationColumn)!;
                edit.insert(new vscode.Position(line, continuationOffset), ' '.repeat(reinsertCodePointLen));
                edit.replace(new vscode.Selection(new vscode.Position(line, continuationOffset), new vscode.Position(line, codepointToOffset(lineText, continuationColumn + 1)!)), contSymbol);
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
    Promise.resolve().then(() => { editor.selections = new_selection; });
}

function extractLineRangesForRemoval(editor: vscode.TextEditor, continuationOffset: number): { start: number, end: number }[] {
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
export function removeContinuation(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number) {
    let new_selection: vscode.Selection[] = [];
    for (const line_range of extractLineRangesForRemoval(editor, continuationOffset).sort((l, r) => { return l.start - r.start; })) {
        edit.delete(
            new vscode.Range(
                new vscode.Position(line_range.start - 1, editor.document.lineAt(line_range.start - 1).text.length),
                new vscode.Position(line_range.end, editor.document.lineAt(line_range.end).text.length)));
        if (!isLineContinued(editor.document, line_range.end, continuationOffset)) {
            const prevLine = editor.document.lineAt(line_range.start - 1).text;
            edit.replace(
                new vscode.Range(
                    new vscode.Position(line_range.start - 1, codepointToOffset(prevLine, continuationOffset)!),
                    new vscode.Position(line_range.start - 1, codepointToOffset(prevLine, continuationOffset + 1)!)
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

export function rearrangeSequenceNumbers(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number) {
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
        const codepoints = [...lineText];

        if (codepoints.length > continuationOffset) {
            const selectionLength = sel.reduce((r, v) => r + (offsetToCodepoint(lineText, v.end.character)! - offsetToCodepoint(lineText, v.start.character)!), 0);
            const notSpace = Math.max(codepoints.lastIndexOf(' '), continuationOffset) + 1;
            const deletionStart = continuationOffset + (notSpace + 1 == codepoints.length || codepoints.length - notSpace > 8 ? 0 : 1);
            if (notSpace - deletionStart < selectionLength) {
                edit.insert(new vscode.Position(line, codepointToOffset(lineText, continuationOffset)!), ' '.repeat(selectionLength - (notSpace - deletionStart)));
            }
            else if (notSpace - deletionStart > selectionLength) {
                // the end of line segment is either a single character, or longer than 8 => assume continuation symbol is present
                // otherwise assume only sequence symbols are present
                if (codepoints.slice(deletionStart + selectionLength, notSpace).every(x => x === ' ')) // only delete spaces!!!
                    edit.delete(
                        new vscode.Range(
                            new vscode.Position(line, codepointToOffset(lineText, deletionStart + selectionLength)!),
                            new vscode.Position(line, codepointToOffset(lineText, notSpace)!)
                        )
                    );
            }
        }
        for (let s of sel.sort((l, r) => r.start.character - l.start.character))
            if (!s.isEmpty)
                edit.delete(s);
    }
}
