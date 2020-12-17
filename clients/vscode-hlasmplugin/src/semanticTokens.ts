/* --------------------------------------------------------------------------------------------
 * Copyright (c) TypeFox. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';

import * as vscode from 'vscode';
import {
	TextDocumentRegistrationOptions, ClientCapabilities, ServerCapabilities, DocumentSelector, Disposable,
} from 'vscode-languageserver-protocol';

import { SemanticTokensResult, SemanticTokensRequest, SemanticTokensServerCapabilities } from './protocol.semanticTokens'

import * as UUID from 'vscode-languageclient/lib/utils/uuid';
import { TextDocumentFeature, BaseLanguageClient } from 'vscode-languageclient';

type scope = string;
type uri = string;

export type DecorationTypeInfo = [vscode.Range[], vscode.TextEditorDecorationType]
export type ScopesMap = Map<scope, DecorationTypeInfo>;
export type EditorColorsMap = Map<uri, ScopesMap>;

export declare type ExtendedServerCapabilities = ServerCapabilities & SemanticTokensServerCapabilities;

export class SemanticTokensFeature extends TextDocumentFeature<TextDocumentRegistrationOptions> {
	protected readonly toDispose: vscode.Disposable[];
	protected decorationTypes: Map<string, vscode.TextEditorDecorationType>;
	protected providerRegistration: Disposable;
	protected legend: vscode.SemanticTokensLegend;
	// map: document -> its decorations (map: scope of symbol -> ranges of scope)
	protected definedEditors: EditorColorsMap;
	constructor(client: BaseLanguageClient) {
		super(client, SemanticTokensRequest.type);
		this.toDispose = [];
		this.decorationTypes = new Map();
		this.definedEditors = new Map();
		this.toDispose.push({ dispose: () => this.definedEditors.clear() });
		this.toDispose.push(vscode.workspace.onDidCloseTextDocument(e => {
			const uri = e.uri.toString();
			if (this.definedEditors.has(uri)) {
				this.definedEditors.delete(uri);
			}
		}));
	}
	protected registerLanguageProvider(options: TextDocumentRegistrationOptions): vscode.Disposable {
		return new vscode.Disposable(() => { });
	}
	
	dispose(): void {
		this.toDispose.forEach(disposable => disposable.dispose());
		super.dispose();
	}

	fillClientCapabilities(capabilities: ClientCapabilities): void {
		if (!!capabilities.textDocument) {
			capabilities.textDocument = {};
		}
	}

	initialize(capabilities: ExtendedServerCapabilities, documentSelector: DocumentSelector): void {
		if (!documentSelector) {
			return;
		}

		if (capabilities.semanticTokensProvider)
			this.legend = capabilities.semanticTokensProvider.legend;
		
		const id = UUID.generateUuid();
		this.register(this.messages, {
			id,
			registerOptions: Object.assign({}, { documentSelector: documentSelector })
		});

	}

	protected editorPredicate(uri: string): (editor: vscode.TextEditor) => boolean {
		const predicateUri = vscode.Uri.parse(uri);
		return (editor: vscode.TextEditor) => editor.document.uri.toString() === predicateUri.toString();
	}

	askForTokens(document: vscode.TextDocument) {
		setTimeout(() => {
			
			this._client.onReady().then(() => this._client.sendRequest('textDocument/semanticTokens/full',{textDocument: {uri: document.uri.toString()}}).then((result: SemanticTokensResult) => {
				if (result.data.length > 0)
					this.applyDecorations(document, result.data);
			}))
		}, 100);
	}

	applyDecorations(document: vscode.TextDocument, data: number[]): EditorColorsMap {
		const parsed = document.uri.toString();
		// find the decorations of current editor
		var decors = this.definedEditors.get(parsed);
		// editor found, clear its decorations
		if (decors == undefined) {
			this.definedEditors.set(parsed, new Map<scope, DecorationTypeInfo>());
			decors = this.definedEditors.get(parsed);
		}

		// reset current decors
		decors.forEach((value: DecorationTypeInfo, key: string) => {
			value[0] = [];
			value[1].dispose();
			value[1] = vscode.window.createTextEditorDecorationType({
				color:  new vscode.ThemeColor('hlasm.'+key)
			});
		});

		var previousRange = new vscode.Range(new vscode.Position(0,0),new vscode.Position(0,0));
		for (var i = 0; i < data.length; i+=5) {
			const scope = this.legend.tokenTypes[data[i+3]];
			if (!decors.has(scope))
				decors.set(scope,[[], vscode.window.createTextEditorDecorationType({
					color:  new vscode.ThemeColor('hlasm.'+scope)
				})]);
			// push ranges
			const line = data[i] + previousRange.start.line;
			const char = (data[i] == 0) ? data[i+1] + previousRange.start.character : data[i+1];

			const currentRange = new vscode.Range(new vscode.Position(line, char), new vscode.Position(line, char + data[i+2]))
			decors.get(scope)[0].push(currentRange);
			previousRange = currentRange;
		}

		this.colorize();
		return this.definedEditors;
	}

	colorize() {
		// update colors from config
		vscode.window.visibleTextEditors.forEach(editor => {
			if (editor.document.languageId == 'hlasm') {
				// find the decorations of current editor
				var decors = this.definedEditors.get(editor.document.uri.toString());
				// draw decorations of visible editor
				if (decors !== undefined) {
					// for each of its saved decorations, draw them
					decors.forEach((value: DecorationTypeInfo) => {
						// set new
						editor.setDecorations(
							value[1]
							, value[0]);
					});
				}
			}
		});
	}

}
