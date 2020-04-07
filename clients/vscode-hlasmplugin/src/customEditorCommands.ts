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

/**
 * Overrides text editor commands of VSCode to handle continuations
 */
export class CustomEditorCommands {
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

    /**
     * Overriden paste to handle continuations
     * Removes spaces in front of the continuation character to compensate for the newly added
     */
    paste(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, args: { text: string }) {
        this.insertChars(editor, edit, args.text);
    }

    /**
     * Overriden type to handle continuations
     * Removes space in front of the continuation character to compensate for the newly added
     */
    type(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, args: { text: string }) {
        this.insertChars(editor, edit, args.text);
    }

    /**
     * Overriden cut to handle continuations
     * Works similarly to deleteLeft
     */
    cut(editor: vscode.TextEditor, edit: vscode.TextEditorEdit) {
        const info = this.initializeInfo(editor);

        // do not cut 0 length sequences 
        if (info.currentPosition.line == editor.selection.anchor.line
            && info.currentPosition.character == editor.selection.anchor.character)
            return;
        const cursorPos = this.removeLeft(editor, edit, info);
        // move the cursor if necessary
        if (info.currentPosition.character == info.continuationOffset)
            setCursor(editor, cursorPos);
    }

    /**
     * Overriden default deleteLeft (backspace) to handle continuations
     * Adds extra space(s) in front of the continuation to compensate for the removed characters
     * Works on single line deletions only
     */
    deleteLeft(editor: vscode.TextEditor, edit: vscode.TextEditorEdit) {
        const info = this.initializeInfo(editor);

        const cursorPos = this.removeLeft(editor, edit, info);
        // move the cursor if necessary
        if (info.currentPosition.character == info.continuationOffset)
            setCursor(editor, cursorPos);
    }

    /**
     * Overriden default deleteRight (delete) to handle continuations
     * Adds extra space(s) in front of the continuation to compensate for the removed characters
     * Works on single line deletions only
     */
    deleteRight(editor: vscode.TextEditor, edit: vscode.TextEditorEdit) {
        const info = this.initializeInfo(editor);

        // size of deleted selection
        const selectionSize = (info.currentPosition.character - editor.selection.anchor.character != 0) ?
            info.currentPosition.character - editor.selection.anchor.character :
            -1;

        // position of cursor after delete
        var newCursorPosition = new vscode.Position(
            info.currentPosition.line,
            info.currentPosition.character - ((selectionSize >= -1) ? selectionSize : 0));

        // there is a continuation and it is after our position, handle it
        if (info.continuationOffset != -1 && info.currentPosition.character < info.continuationOffset && editor.selection.isSingleLine) {
            const beforeContinuationChars =
                new vscode.Range(info.currentPosition.line,
                    info.continuationOffset - Math.abs(selectionSize),
                    info.currentPosition.line,
                    info.continuationOffset);

            edit.insert(beforeContinuationChars.end, " ".repeat(Math.abs(selectionSize)));
        }

        // change the cursor if needed
        if (editor.document.lineAt(info.currentPosition).text.substring(info.currentPosition.character) == "")
            newCursorPosition = new vscode.Position(info.currentPosition.line + 1, 0);

        // delete as default
        edit.delete(new vscode.Range(info.currentPosition,
            (editor.selection.anchor.character == info.currentPosition.character &&
                editor.selection.anchor.line == info.currentPosition.line) ?
                newCursorPosition :
                new vscode.Position(editor.selection.anchor.line, editor.selection.anchor.character)));
    }
    /**
     * Common logic for both type and paste
     * @param editor 
     * @param edit 
     */
    private insertChars(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, text: string) {
        const info = this.initializeInfo(editor);

        // typing with multiple characters selected
        if (info.currentPosition.line != editor.selection.anchor.line || info.currentPosition.character != editor.selection.anchor.character) {
            // different cases of insert, depending on the anchor/active positions
            if (editor.selection.anchor.line > editor.selection.active.line)
                edit.insert(editor.selection.active, text);
            else if (editor.selection.anchor.line < editor.selection.active.line)
                edit.insert(editor.selection.anchor, text);
            else {
                if (editor.selection.anchor.character < editor.selection.active.character)
                    edit.insert(editor.selection.anchor, text);
                else
                    edit.insert(editor.selection.active, text);
            }

            //simulate delete function for the selected characters
            this.removeLeft(editor, edit, info);
        }
        // simple single character insert
        else
            edit.insert(info.currentPosition, text);
        // there is a continuation, prepare space for new characters
        if (info.continuationOffset != -1 && info.currentPosition.character < info.continuationOffset) {
            // find free space in front of it
            const spaceRange = this.findSpace(editor.document.lineAt(info.currentPosition), text.length, info);
            // if there is, delete it
            if (spaceRange)
                edit.delete(spaceRange);
        }
    }

    private removeLeft(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, info: LinePositionsInfo) {
        // size of deleted selection
        const selectionSize =
            (info.currentPosition.character - editor.selection.anchor.character != 0)
                ? info.currentPosition.character - editor.selection.anchor.character
                : 1;

        // position of cursor after delete
        var newCursorPosition =
            new vscode.Position(
                info.currentPosition.line,
                info.currentPosition.character - ((selectionSize > 0 && info.currentPosition.character > 0) ? selectionSize : 0));

        //end of selection
        const endPos =
            (info.currentPosition.character < editor.selection.anchor.character)
                ? editor.selection.anchor.character
                : info.currentPosition.character;

        // there is a continuation and it is after our position, handle it
        if (info.continuationOffset != -1 && endPos < info.continuationOffset && info.currentPosition.character > 0 && editor.selection.isSingleLine) {
            const beforeContinuationChars = new vscode.Range(
                info.currentPosition.line, info.continuationOffset - Math.abs(selectionSize),
                info.currentPosition.line, info.continuationOffset);

            edit.insert(beforeContinuationChars.end, " ".repeat(Math.abs(selectionSize)));
        }
        // special case for delete at the beginning of line
        else if (info.currentPosition.character == 0 && info.currentPosition.line > 0) {
            newCursorPosition = new vscode.Position(
                info.currentPosition.line - 1,
                editor.document.lineAt(info.currentPosition.line - 1).text.length);
        }
        // delete as default
        edit.delete(new vscode.Range(info.currentPosition,
            (editor.selection.anchor.character == info.currentPosition.character
                && editor.selection.anchor.line == info.currentPosition.line) ?
                newCursorPosition :
                new vscode.Position(editor.selection.anchor.line, editor.selection.anchor.character)));

        return newCursorPosition;
    }

    private initializeInfo(editor: vscode.TextEditor): LinePositionsInfo {
        return new LinePositionsInfo(
            editor.selection.active,
            this.highlight.getContinuation(editor.selection.active.line, editor.document.uri.toString())
        );
    }

    // find space for line, used for custom type
    private findSpace(textLine: vscode.TextLine, length: number, info: LinePositionsInfo) {
        var spacePosition = info.continuationOffset - 1;
        var currentSymbol = textLine.text[spacePosition];
        // go backwards through all the symbols since the continuation and check whether there are enough spaces to compensate for the input
        while (currentSymbol == " " && spacePosition > info.currentPosition.character && length > 0) {
            spacePosition--;
            length--;
            currentSymbol = textLine.text[spacePosition];
        }
        return (length > 0)
            ? null
            : new vscode.Range(textLine.lineNumber, spacePosition, textLine.lineNumber, info.continuationOffset - 1);
    }
}

export function setCursor(editor: vscode.TextEditor, position: vscode.Position) {
    editor.selection = new vscode.Selection(position, position);
}

// helper class to store continuation information
export class LinePositionsInfo {
    currentPosition: vscode.Position;
    continuationOffset: number;

    // initializes continuation vars
    constructor(currentPosition: vscode.Position, continuationOffset: number) {
        this.currentPosition = currentPosition;
        this.continuationOffset = continuationOffset;
    }
}