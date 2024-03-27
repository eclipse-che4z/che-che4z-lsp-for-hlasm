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

import * as vscode from 'vscode';
import { continuationColumn } from './constants';

export enum CommentOption {
    toggle,
    add,
    remove,
}
export function translateCommentOption(text: string): CommentOption | null {
    if (text === 'CommentOption.toggle')
        return CommentOption.toggle;
    if (text === 'CommentOption.add')
        return CommentOption.add;
    if (text === 'CommentOption.remove')
        return CommentOption.remove;

    return null;
}

function seq(low: number, high: number): number[] {
    return Array.from({ length: high + 1 - low }, (_, i) => i + low);
}

function isContinued(text: string): boolean {
    return text.length > continuationColumn && ([...text][continuationColumn] ?? ' ') !== ' ';
}

function findFirstLine(doc: vscode.TextDocument, lineno: number): number {
    while (lineno > 0 && isContinued(doc.lineAt(lineno - 1).text))
        --lineno;

    return lineno;
}

function findLastLine(doc: vscode.TextDocument, lineno: number): number {
    while (lineno < doc.lineCount && isContinued(doc.lineAt(lineno).text))
        ++lineno;

    return lineno;
}

function isCommented(doc: vscode.TextDocument, lineno: number): boolean {
    const text = doc.lineAt(lineno).text;
    return text.startsWith('*') || text.startsWith('.*');
}

function addComment(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, lineno: number) {
    if (editor.document.lineAt(lineno).text.length > 0)
        edit.replace(new vscode.Selection(new vscode.Position(lineno, 0), new vscode.Position(lineno, 1)), '*');
    else
        edit.insert(new vscode.Position(lineno, 0), '*');
}

function removeComment(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, lineno: number) {
    const text = editor.document.lineAt(lineno).text;
    let replacement = ' ';
    let i = 1;
    for (; i < text.length; ++i) {
        if (text.charAt(i) !== '*') {
            replacement = text.charAt(i);
            break;
        }
    }
    replacement = replacement.repeat(i);

    edit.replace(new vscode.Selection(new vscode.Position(lineno, 0), new vscode.Position(lineno, i)), replacement);
}

export function lineCommentCommand(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, args: CommentOption) {
    if (editor.document.isClosed) return;

    const lines = [
        ...new Set(
            editor.selections
                .flatMap(x => seq(x.start.line, x.end.line - +(x.start.line != x.end.line && x.end.character == 0)))
                .map(x => findFirstLine(editor.document, x))
        )
    ].sort((l, r) => l - r);
    const lineWithStatus = lines.map(x => { return { lineno: x, commented: isCommented(editor.document, x) }; })

    if (args == CommentOption.toggle) {
        if (lineWithStatus.every(x => x.commented))
            args = CommentOption.remove;
        else
            args = CommentOption.add;
    }

    for (const { lineno, commented } of lineWithStatus) {
        switch (args) {
            case CommentOption.add:
                if (!commented)
                    addComment(editor, edit, lineno);
                break;

            case CommentOption.remove:
                if (commented)
                    removeComment(editor, edit, lineno);
                break;
        }
    }
}

interface CodeBlock {
    first: number;
    last: number;
}

function isolateBlocks(block_candidates: CodeBlock[]): CodeBlock[] {
    let last_last = -1;
    const blocks: CodeBlock[] = [];

    for (const b of block_candidates) {
        if (b.first > last_last)
            blocks.push(b);
        else
            blocks.at(-1)!.last = Math.max(b.last, last_last);

        last_last = blocks.at(-1)!.last;
    }

    return blocks;
}

function processBlock(doc: vscode.TextDocument, b: CodeBlock): {
    addComment: true;
    commentArea: CodeBlock;
} | {
    addComment: false;
    removeLines: {
        begin: number;
        end: number;
    }[];
} {
    const begin = /^ +AGO +(\.[A-Z@#$_][A-Z@#$0-9_]*)(:? .+)?/i;

    let start_line = b.first;
    let bm = begin.exec(doc.lineAt(start_line).text);
    if (!bm && start_line > 0) {
        const prev_line = findFirstLine(doc, start_line - 1);

        bm = begin.exec(doc.lineAt(prev_line).text);
        if (bm)
            start_line = prev_line;
    }
    if (!bm)
        return { commentArea: b, addComment: true };

    const end = new RegExp('^' + bm[1] + '[ ]+ANOP(?: .+)?', 'i');
    let em = end.exec(doc.lineAt(findFirstLine(doc, b.last)).text);
    let end_line = b.last;
    if (!em && b.last + 1 < doc.lineCount) {
        em = end.exec(doc.lineAt(b.last + 1).text);
        if (!em)
            return { commentArea: b, addComment: true };

        end_line = findLastLine(doc, b.last + 1);
    }

    return {
        addComment: false,
        removeLines: [
            { begin: start_line, end: findLastLine(doc, start_line) },
            { begin: findFirstLine(doc, end_line), end: end_line }
        ]
    };
}

export function blockCommentCommand(editor: vscode.TextEditor, edit: vscode.TextEditorEdit) {
    if (editor.document.isClosed) return;

    const block_candidates = [...new Set(editor.selections.flatMap(x => {
        return {
            first: findFirstLine(editor.document, x.start.line),
            last: findLastLine(editor.document, x.end.line - +(x.start.line != x.end.line && x.end.character == 0))
        }
    }))].sort((l, r) => l.first - r.first || -(l.last - r.last));

    const blocks = isolateBlocks(block_candidates).map(x => processBlock(editor.document, x));

    const eol = editor.document.eol == vscode.EndOfLine.LF ? '\n' : '\r\n';
    for (const b of blocks) {
        if (b.addComment) {
            const label = '.SKIP_' + b.commentArea.first + '_' + b.commentArea.last;
            edit.insert(new vscode.Position(b.commentArea.first, 0), '         AGO   ' + label + eol);
            if (b.commentArea.last + 1 < editor.document.lineCount)
                edit.insert(new vscode.Position(b.commentArea.last + 1, 0), label + ' ANOP' + eol);
            else
                edit.insert(editor.document.lineAt(b.commentArea.last).range.end, eol + label + ' ANOP');
        } else {
            for (const sub of b.removeLines)
                edit.delete(new vscode.Range(new vscode.Position(sub.begin, 0), new vscode.Position(sub.end + 1, 0)))
        }
    }
}
