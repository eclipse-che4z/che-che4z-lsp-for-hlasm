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

/**
 * Overrides text editor commands of VSCode to handle continuations
 */
export class CustomEditorCommands {
    dispose() {}

    /**
     * Overriden cut to handle continuations
     * Works similarly to deleteLeft
     */
    cut(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number) {
        // do not cut 0 length sequences 
        if (editor.selection.active.line == editor.selection.anchor.line
            && editor.selection.active.character == editor.selection.anchor.character)
            return;
        const cursorPos = this.removeLeft(editor, edit, continuationOffset);
        // move the cursor if necessary
        if (editor.selection.active.character == continuationOffset)
            setCursor(editor, cursorPos);
    }

    /**
     * Overriden default deleteLeft (backspace) to handle continuations
     * Adds extra space(s) in front of the continuation to compensate for the removed characters
     * Works on single line deletions only
     */
    deleteLeft(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number) {
        const cursorPos = this.removeLeft(editor, edit, continuationOffset);
        // move the cursor if necessary
        if (editor.selection.active.character == continuationOffset)
            setCursor(editor, cursorPos);
    }

    /**
     * Overriden default deleteRight (delete) to handle continuations
     * Adds extra space(s) in front of the continuation to compensate for the removed characters
     * Works on single line deletions only
     */
    deleteRight(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number) {
        // size of deleted selection
        const selectionSize = (editor.selection.active.character - editor.selection.anchor.character != 0) ?
            editor.selection.active.character - editor.selection.anchor.character :
            -1;

        // position of cursor after delete
        var newCursorPosition = new vscode.Position(
            editor.selection.active.line,
            editor.selection.active.character - ((selectionSize >= -1) ? selectionSize : 0));

        // there is a continuation and it is after our position, handle it
        if (continuationOffset && editor.selection.active.character < continuationOffset && editor.selection.isSingleLine) {
            const beforeContinuationChars =
                new vscode.Range(editor.selection.active.line,
                    continuationOffset - Math.abs(selectionSize),
                    editor.selection.active.line,
                    continuationOffset);

            edit.insert(beforeContinuationChars.end, " ".repeat(Math.abs(selectionSize)));
        }

        // change the cursor if needed
        if (editor.document.lineAt(editor.selection.active).text.substring(editor.selection.active.character) == "")
            newCursorPosition = new vscode.Position(editor.selection.active.line + 1, 0);

        // delete as default
        edit.delete(new vscode.Range(editor.selection.active,
            (editor.selection.anchor.character == editor.selection.active.character &&
                editor.selection.anchor.line == editor.selection.active.line) ?
                newCursorPosition :
                new vscode.Position(editor.selection.anchor.line, editor.selection.anchor.character)));
    }

    /**
     * Common logic for both type and paste
     * Removes space in front of the continuation character to compensate for the newly added
     * @param editor 
     * @param edit 
     */
    insertChars(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, args: {text: string}, continuationOffset: number) {
        // typing with multiple characters selected
        if (editor.selection.active.line != editor.selection.anchor.line || editor.selection.active.character != editor.selection.anchor.character) {
            //simulate delete function for the selected characters
            this.removeLeft(editor, edit, continuationOffset);
            // different cases of insert, depending on the anchor/active positions
            if (editor.selection.anchor.line > editor.selection.active.line)
                edit.insert(editor.selection.active, args.text);
            else if (editor.selection.anchor.line < editor.selection.active.line)
                edit.insert(editor.selection.anchor, args.text);
            else {
                if (editor.selection.anchor.character < editor.selection.active.character)
                    edit.insert(editor.selection.anchor, args.text);
                else
                    edit.insert(editor.selection.active, args.text);
            }
        }
        // simple single character insert
        else
            edit.insert(editor.selection.active, args.text);
        // there is a continuation, prepare space for new characters
        if (continuationOffset && editor.selection.active.character < continuationOffset) {
            // find free space in front of it
            const spaceRange = this.findSpace(editor.document.lineAt(editor.selection.active), args.text.length, editor,continuationOffset);
            // if there is, delete it
            if (spaceRange)
                edit.delete(spaceRange);
        }
    }

    private removeLeft(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, continuationOffset: number) {
        // size of deleted selection
        const selectionSize =
            (editor.selection.active.character - editor.selection.anchor.character != 0)
                ? editor.selection.active.character - editor.selection.anchor.character
                : 1;

        // position of cursor after delete
        var newCursorPosition =
            new vscode.Position(
                editor.selection.active.line,
                editor.selection.active.character - ((selectionSize > 0 && editor.selection.active.character > 0) ? selectionSize : 0));

        //end of selection
        const endPos =
            (editor.selection.active.character < editor.selection.anchor.character)
                ? editor.selection.anchor.character
                : editor.selection.active.character;

        // there is a continuation and it is after our position, handle it
        if (continuationOffset && endPos < continuationOffset && (editor.selection.active.character > 0 || selectionSize > 0) && editor.selection.isSingleLine) {
            const beforeContinuationChars = new vscode.Range(
                editor.selection.active.line, continuationOffset - Math.abs(selectionSize),
                editor.selection.active.line, continuationOffset);

            edit.insert(beforeContinuationChars.end, " ".repeat(Math.abs(selectionSize)));
        }
        // special case for delete at the beginning of line
        else if (editor.selection.active.character == 0 && editor.selection.active.line > 0) {
            newCursorPosition = new vscode.Position(
                editor.selection.active.line - 1,
                editor.document.lineAt(editor.selection.active.line - 1).text.length);
        }
        // delete as default
        edit.delete(new vscode.Range(editor.selection.active,
            (editor.selection.anchor.character == editor.selection.active.character
                && editor.selection.anchor.line == editor.selection.active.line) ?
                newCursorPosition :
                new vscode.Position(editor.selection.anchor.line, editor.selection.anchor.character)));

        return newCursorPosition;
    }

    // find space for line, used for custom type
    private findSpace(textLine: vscode.TextLine, length: number, editor: vscode.TextEditor, continuationOffset: number) {
        var spacePosition = continuationOffset - 1;
        var currentSymbol = textLine.text[spacePosition];
        // go backwards through all the symbols since the continuation and check whether there are enough spaces to compensate for the input
        while (currentSymbol == " " && spacePosition > editor.selection.active.character && length > 0) {
            spacePosition--;
            length--;
            currentSymbol = textLine.text[spacePosition];
        }
        return (length > 0)
            ? null
            : new vscode.Range(textLine.lineNumber, spacePosition, textLine.lineNumber, continuationOffset - 1);
    }
}

export function setCursor(editor: vscode.TextEditor, position: vscode.Position) {
    editor.selection = new vscode.Selection(position, position);
}
