/* --------------------------------------------------------------------------------------------
 * Copyright (c) TypeFox. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';

import { Disposable, Uri, Range, window, DecorationRenderOptions, TextEditorDecorationType, workspace, TextEditor, Position } from 'vscode';
import {
	TextDocumentRegistrationOptions, ClientCapabilities, ServerCapabilities, DocumentSelector, NotificationHandler,
} from 'vscode-languageserver-protocol';

import {SemanticHighlightingNotification, SemanticHighlightingParams, SemanticHighlightingInformation} from './protocol.semanticHighlighting'

import * as UUID from '../node_modules/vscode-languageclient/lib/utils/uuid';
import { TextDocumentFeature, BaseLanguageClient } from 'vscode-languageclient';

export class SemanticHighlightingFeature extends TextDocumentFeature<TextDocumentRegistrationOptions> {

	protected readonly toDispose: Disposable[];
	protected readonly decorations: Map<string, any>;
	protected readonly handlers: NotificationHandler<SemanticHighlightingParams>[];
	protected decorationTypes: decorationType[];
	protected definedEditors: editorDecorations[];
	protected continuedLines: continuedLine[]; 
	protected continueColumnPosition: Map<string,number>;
	constructor(client: BaseLanguageClient) {
		super(client, SemanticHighlightingNotification.type);
		this.toDispose = [];
		this.decorations = new Map();
		this.handlers = [];
		this.decorationTypes = [];
		this.definedEditors = [];
		this.continuedLines = []
		this.continueColumnPosition = new Map();
		this.toDispose.push({ dispose: () => this.decorations.clear() });
		this.toDispose.push(workspace.onDidCloseTextDocument(e => {
			const uri = e.uri.toString();
			if (this.decorations.has(uri)) {
				// TODO: do the proper disposal of the decorations.
				this.decorations.delete(uri);
			}
		}));
	}

	dispose(): void {
		this.toDispose.forEach(disposable => disposable.dispose());
		super.dispose();
	}

	fillClientCapabilities(capabilities: ClientCapabilities): void {

	}


	initialize(capabilities: ServerCapabilities, documentSelector: DocumentSelector): void {
		if (!documentSelector) {
			return;
		}
		this.updateColors();
		const id = UUID.generateUuid();
		this.register(this.messages, {
			id,
			registerOptions: Object.assign({}, { documentSelector: documentSelector })
		});
	}
	getColor(name: string)
    {
		var found = workspace.getConfiguration().hlasmplugin.highlightColors.find((color: any) => color.id == name);
		if (found)
			return found.hex;
		else
			return "#ffffff";
    }
	protected registerLanguageProvider(options: TextDocumentRegistrationOptions): Disposable {
		if (options.documentSelector === null) {
			return new Disposable(() => { });
		}
		const handler = this.newNotificationHandler.bind(this)();
		this._client.onNotification(SemanticHighlightingNotification.type, handler);
		return new Disposable(() => {
			const indexOf = this.handlers.indexOf(handler);
			if (indexOf !== -1) {
				this.handlers.splice(indexOf, 1);
			}
		})
	}

	protected newNotificationHandler(): NotificationHandler<SemanticHighlightingParams> {
		return (params) => {
            this.applyDecorations(params);
        };
	}

	protected editorPredicate(uri: string): (editor: TextEditor) => boolean {
		const predicateUri = Uri.parse(uri);
		return (editor: TextEditor) => editor.document.uri.toString() === predicateUri.toString();
	}

	public applyDecorations(params?: SemanticHighlightingParams): void {
		if (params)
		{
			var tempMap = new Map();
			this.continueColumnPosition.set(params.textDocument.uri, params.continuationPositions[0][1]);
			tempMap.set(-1, params.continuationPositions[0][0]);
			params.continuationPositions.shift();
			params.continuationPositions.forEach((position: number[]) => {
				tempMap.set(position[0],position[1]);
			});
			var continuationIndex = this.continuedLines.findIndex((continuedLine) => params.textDocument.uri == continuedLine.documentUri);
			if (continuationIndex != -1)
				this.continuedLines[continuationIndex].continuationPositions = tempMap;
			else
				this.continuedLines.push(new continuedLine(params.textDocument.uri, tempMap));
		}
		window.visibleTextEditors.forEach(editor =>
		{
			var editorIndex = this.definedEditors.findIndex(definedEditor => definedEditor.documentUri == editor.document.uri.toString());
			if (params && params.textDocument.uri == editor.document.uri.toString())
			{
				if (editorIndex != -1)
				{
					this.definedEditors[editorIndex].decorationRanges.clear();
				}
				else
				{
					this.definedEditors.push(new editorDecorations(params.textDocument.uri));
					editorIndex = this.definedEditors.length - 1;
				}
				this.decorationTypes.forEach(type => {
					this.definedEditors[editorIndex].decorationRanges.set(type.scope, []);
				});
				params.tokens.forEach(token => {
					var lexOffset = (token.scope == "continuation" || token.scope == "ignored" || token.scope == "comment") ? 1 : 0;
					this.definedEditors[editorIndex].decorationRanges.get(token.scope)!.push(new Range(new Position(token.lineStart, token.columnStart), new Position(token.lineEnd, token.columnEnd + lexOffset)));
				});
			}
			
			if (editorIndex != -1)
			{
				this.definedEditors[editorIndex].decorationRanges.forEach((value: Range[], key: string) =>
				{
					editor.setDecorations(this.decorationTypes.find(type => type.scope == key)!.type, value);
				});
			}
		});
	}

    public updateColors()
    {
        this.decorationTypes = [
			new decorationType("comment", window.createTextEditorDecorationType({ color: this.getColor("comment") })),
			new decorationType("ignored", window.createTextEditorDecorationType({ color: this.getColor("ignored") })),
			new decorationType("continuation", window.createTextEditorDecorationType({ color: this.getColor("continuation") })),
			new decorationType("remark", window.createTextEditorDecorationType({ color: this.getColor("remark") })),
			new decorationType("varSymbol", window.createTextEditorDecorationType({ color: this.getColor("varSymbol") })),
			new decorationType("string", window.createTextEditorDecorationType({ color: this.getColor("string") })),
			new decorationType("number", window.createTextEditorDecorationType({ color: this.getColor("number") })),
			new decorationType("seqSymbol", window.createTextEditorDecorationType({ color: this.getColor("seqSymbol") })),
			new decorationType("label", window.createTextEditorDecorationType({ color: this.getColor("label") })),
			new decorationType("operator", window.createTextEditorDecorationType({ color: this.getColor("operator") })),
			new decorationType("instruction", window.createTextEditorDecorationType({ color: this.getColor("instruction") })),
			new decorationType("operand", window.createTextEditorDecorationType({ color: this.getColor("operand") }))
		];
    }

	protected decorationType(options: DecorationRenderOptions = {}) {
		return window.createTextEditorDecorationType(options);
	}

	protected map2Decoration(lines: SemanticHighlightingInformation[]): [TextEditorDecorationType, Range[]] {
		console.log('TODO: Map the lines (and the tokens) to the desired decoration type.', lines);
		return [this.decorationType(), []];
	}
	public getContinuation(line: number, uri: string) {
		var foundLine = this.continuedLines.find((continuedLine) => continuedLine.documentUri == uri);
		return (foundLine && foundLine.continuationPositions.get(line)) ? foundLine.continuationPositions.get(line) : -1;
	}
	public getContinueColumn(uri: string)
	{
		return this.continueColumnPosition.get(uri);
	}
}

class editorDecorations
{
	public documentUri: string;
	public decorationRanges: Map<string, Range[]>
    constructor(documentUri: string)
    {
        this.documentUri = documentUri;
        this.decorationRanges = new Map();
    }
}
class decorationType {
	public scope: string;
	public type: TextEditorDecorationType;
    constructor(scope: string, type:TextEditorDecorationType)
    {
        this.scope = scope;
        this.type = type;
	}
}
class continuedLine {
	public documentUri: string;
	public continuationPositions: Map<number,number>;
    constructor(documentUri: string, continuationPositions: Map<number,number>)
    {
		this.documentUri = documentUri;
		this.continuationPositions = continuationPositions;
	}
}
