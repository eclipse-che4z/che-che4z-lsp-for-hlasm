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
import * as vscodelc from 'vscode-languageclient/node';
/**
 * A collection of mocked interfaces needed for unit testing
 */

export class LanguageClientOptionsMock implements vscodelc.LanguageClientOptions {
}

export class LanguageClientMock extends vscodelc.BaseLanguageClient {
    protected createMessageTransports(encoding: string): Promise<vscodelc.MessageTransports> {
        throw new Error("Method not implemented.");
    }
    protected getLocale(): string { return "" };
}

export class TextDocumentChangeEventMock implements vscode.TextDocumentChangeEvent {
    constructor(changes: vscode.TextDocumentContentChangeEvent[]) {
        this.contentChanges = changes;
    }
    document: vscode.TextDocument;
    contentChanges: readonly vscode.TextDocumentContentChangeEvent[];
    reason: undefined;
}

export class TextDocumentContentChangeEventMock implements vscode.TextDocumentContentChangeEvent {
    range: vscode.Range;
    rangeOffset: number;
    rangeLength: number;
    text: string;
}

export class ConfigurationChangeEventMock implements vscode.ConfigurationChangeEvent {
    affectsConfiguration(section: string, scope?: vscode.ConfigurationScope): boolean {
        return false;
    }
}

export class DebugConfigurationMock implements vscode.DebugConfiguration {
    [key: string]: any;
    type: string;
    name: string;
    request: string;
}

export class TextEditorMock implements vscode.TextEditor {
    constructor(document: vscode.TextDocument) {
        this.selections = [];
        this.document = document;
    }
    document: vscode.TextDocument;
    selections: vscode.Selection[];

    public get selection(): vscode.Selection {
        return this.selections[0];
    }
    public set selection(v: vscode.Selection) {
        this.selections = [v];
    }

    visibleRanges: vscode.Range[];
    options: vscode.TextEditorOptions;
    viewColumn: vscode.ViewColumn;
    edit(callback: (editBuilder: vscode.TextEditorEdit) => void, options?: { undoStopBefore: boolean; undoStopAfter: boolean; }): Thenable<boolean> {
        throw new Error("Method not implemented.");
    }
    insertSnippet(snippet: vscode.SnippetString, location?: vscode.Position | vscode.Range | readonly vscode.Position[] | readonly vscode.Range[], options?: { undoStopBefore: boolean; undoStopAfter: boolean; }): Thenable<boolean> {
        throw new Error("Method not implemented.");
    }
    setDecorations(decorationType: vscode.TextEditorDecorationType, rangesOrOptions: vscode.Range[] | vscode.DecorationOptions[]): void {
        throw new Error("Method not implemented.");
    }
    revealRange(range: vscode.Range, revealType?: vscode.TextEditorRevealType): void {
        throw new Error("Method not implemented.");
    }
    show(column?: vscode.ViewColumn): void {
        throw new Error("Method not implemented.");
    }
    hide(): void {
        throw new Error("Method not implemented.");
    }
}

export class TextEditorEditMock implements vscode.TextEditorEdit {
    private static get_pos(loc: vscode.Range | vscode.Position | vscode.Selection) {
        if (loc instanceof vscode.Selection) return loc.active;
        if (loc instanceof vscode.Range) return loc.start;
        if (loc instanceof vscode.Position) return loc;
        return null;
    }
    private thing_to_do: (
        { loc: vscode.Range | vscode.Position | vscode.Selection, value: string | null, what: 'replace' }
        |
        { loc: vscode.Position, value: string | null, what: 'insert' }
        |
        { loc: vscode.Range | vscode.Selection, value: string | null, what: 'delete' }
    )[] = [];
    private _text: string;
    public get text(): string {
        this.thing_to_do.sort((x, y) => -TextEditorEditMock.get_pos(x.loc).compareTo(TextEditorEditMock.get_pos(y.loc)));
        for (const action of this.thing_to_do) {
            switch (action.what) {
                case 'replace':
                    this._replace(action.loc, action.value);
                    break;
                case 'insert':
                    this._insert(action.loc, action.value);
                    break;
                case 'delete':
                    this._delete(action.loc)
                    break;
            }
        }
        this.thing_to_do = [];
        return this._text;
    }

    constructor(text: string) {
        this._text = text;
    }

    private skipLines(text: string, line: number): number {
        let pos = 0;
        for (let i = 0; i < line; ++i) {
            pos = this._text.indexOf('\n', pos);
            if (pos < 0) {
                if (i + 1 === line) return text.length;
                throw new Error("Bad coordinate");
            }
            pos += 1
        }
        return pos;
    }

    replace(location: vscode.Range | vscode.Position | vscode.Selection, value: string): void {
        this.thing_to_do.push({ loc: location, value: value, what: 'replace' });
    }
    insert(location: vscode.Position, value: string): void {
        this.thing_to_do.push({ loc: location, value: value, what: 'insert' });
    }
    delete(location: vscode.Range | vscode.Selection): void {
        this.thing_to_do.push({ loc: location, value: null, what: 'delete' });
    }

    _replace(location: vscode.Range | vscode.Position | vscode.Selection, value: string): void {
        this._delete(location as vscode.Range);
        this._insert((location as vscode.Range).start, value);
    }
    _insert(location: vscode.Position, value: string): void {
        let pos = this.skipLines(this._text, location.line);
        var before = this._text.slice(0, pos + location.character);
        var after = this._text.slice(pos + location.character);
        this._text = before + value + after;
    }
    _delete(location: vscode.Range | vscode.Selection): void {
        let pos_start = this.skipLines(this._text, location.start.line);
        let pos_end = this.skipLines(this._text, location.end.line);
        var before = this._text.slice(0, pos_start + location.start.character);
        var after = this._text.slice(pos_end + location.end.character);
        this._text = before + after;
    }
    setEndOfLine(endOfLine: vscode.EndOfLine): void {
        throw new Error("Method not implemented.");
    }

}

class TextLineMock implements vscode.TextLine {
    lineNumber: number;
    text: string;
    range: vscode.Range;
    rangeIncludingLineBreak: vscode.Range;
    firstNonWhitespaceCharacterIndex: number;
    isEmptyOrWhitespace: boolean;
}

export class TextDocumentMock implements vscode.TextDocument {
    text: string;
    uri: vscode.Uri;
    fileName: string;
    isUntitled: boolean;
    languageId: string;
    version: number;
    isDirty: boolean;
    isClosed: boolean;
    save(): Thenable<boolean> {
        throw new Error("Method not implemented.");
    }
    eol: vscode.EndOfLine;

    public get lineCount(): number {
        let result = 0;
        let pos = 0;
        do {
            ++result;
            pos = this.text.indexOf('\n', pos) + 1;
        } while (pos != 0)

        return result;
    }

    lineAt(line: number): vscode.TextLine;
    lineAt(position: vscode.Position): vscode.TextLine;
    lineAt(position: any) {
        var line = new TextLineMock();
        var lines = this.text.split(/\r?\n/);
        if ((position as vscode.Position).line !== undefined)
            line.lineNumber = (position as vscode.Position).line;
        else
            line.lineNumber = position as number;
        line.text = lines[line.lineNumber] || '';
        line.range = new vscode.Range(line.lineNumber, 0, line.lineNumber, line.text.length);
        return line;
    }
    offsetAt(position: vscode.Position): number {
        throw new Error("Method not implemented.");
    }
    positionAt(offset: number): vscode.Position {
        throw new Error("Method not implemented.");
    }
    getText(range?: vscode.Range): string {
        return this.text;
    }
    getWordRangeAtPosition(position: vscode.Position, regex?: RegExp): vscode.Range {
        throw new Error("Method not implemented.");
    }
    validateRange(range: vscode.Range): vscode.Range {
        throw new Error("Method not implemented.");
    }
    validatePosition(position: vscode.Position): vscode.Position {
        if (this.text.length > position.character)
            return position;
        else
            return new vscode.Position(0, this.text.length);
    }
}
