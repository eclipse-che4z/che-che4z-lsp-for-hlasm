/* --------------------------------------------------------------------------------------------
 * Copyright (c) TypeFox. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';

import * as vscode from 'vscode';
import {
	TextDocumentRegistrationOptions, ClientCapabilities, ServerCapabilities, DocumentSelector, NotificationHandler,
} from 'vscode-languageserver-protocol';

import { SemanticHighlightingNotification, SemanticHighlightingParams, SemanticHighlightingClientCapabilities } from './protocol.semanticHighlighting'

import * as UUID from '../node_modules/vscode-languageclient/lib/utils/uuid';
import { TextDocumentFeature, BaseLanguageClient, LanguageClient } from 'vscode-languageclient';

export declare type ExtendedClientCapabilities = ClientCapabilities & SemanticHighlightingClientCapabilities;

/**
 * Client extended by Semantic Highlighting feature
 */
export class ExtendedLanguageClient extends LanguageClient {
	semanticHighlighting: SemanticHighlightingFeature;
	protected registerBuiltinFeatures(): void {
		var semanticHighlighting = new SemanticHighlightingFeature(this);
		super.registerBuiltinFeatures();
		this.registerFeature(semanticHighlighting);
		this.semanticHighlighting = semanticHighlighting;
	}

	public applyDecorations(params: SemanticHighlightingParams) {
		this.semanticHighlighting.applyDecorations(params);
	}
}

export { SemanticHighlightingParams }

export * from 'vscode-languageclient'

type scope = string;
type uri = string;

export type ScopeRangeMap = Map<scope, vscode.Range[]>;
export type EditorColorsMap = Map<uri, ScopeRangeMap>;

export class SemanticHighlightingFeature extends TextDocumentFeature<TextDocumentRegistrationOptions> {

	protected readonly toDispose: vscode.Disposable[];
	protected readonly handlers: NotificationHandler<SemanticHighlightingParams>[];
	// map of possible decoration types: scope of symbol -> color of scope
	protected decorationTypes: Map<scope, DecorationType>;
	// map: document -> its decorations (map: scope of symbol -> ranges of scope)
	protected definedEditors: EditorColorsMap;
	constructor(client: BaseLanguageClient) {
		super(client, SemanticHighlightingNotification.type);
		this.toDispose = [];
		this.handlers = [];
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

	dispose(): void {
		this.toDispose.forEach(disposable => disposable.dispose());
		super.dispose();
	}

	fillClientCapabilities(capabilities: ExtendedClientCapabilities): void {
		if (!!capabilities.textDocument) {
			capabilities.textDocument = {};
		}
		capabilities.textDocument!.semanticHighlightingCapabilities = {
			semanticHighlighting: true
		};
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

	protected registerLanguageProvider(options: TextDocumentRegistrationOptions): vscode.Disposable {
		if (options.documentSelector === null) {
			return new vscode.Disposable(() => { });
		}
		const handler = this.newNotificationHandler.bind(this)();
		this._client.onNotification(SemanticHighlightingNotification.type, handler);
		return new vscode.Disposable(() => {
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

	protected editorPredicate(uri: string): (editor: vscode.TextEditor) => boolean {
		const predicateUri = vscode.Uri.parse(uri);
		return (editor: vscode.TextEditor) => editor.document.uri.toString() === predicateUri.toString();
	}

	applyDecorations(params: SemanticHighlightingParams): EditorColorsMap {
		const parsed = vscode.Uri.parse(params.textDocument.uri).toString();
		// find the decorations of current editor
		var decors = this.definedEditors.get(parsed);
		// editor found, clear its decorations
		if (decors == undefined) {
			this.definedEditors.set(parsed, new Map<scope, vscode.Range[]>());
			decors = this.definedEditors.get(parsed);
		}
		// clear ranges
		Array.from(this.decorationTypes.keys()).forEach(scope => {
			decors.set(scope, []);
		})
		// add range for each token to its corresponding scope
		params.tokens.forEach(token => {
			decors.get(token.scope)!.push(new vscode.Range(new vscode.Position(token.lineStart, token.columnStart), new vscode.Position(token.lineEnd, token.columnEnd)));
		});
		this.colorize();
		return this.definedEditors;
	}

	colorize() {
		// update colors from config
		this.updateColors();
		vscode.window.visibleTextEditors.forEach(editor => {
			if (editor.document.languageId == 'hlasm') {
				// find the decorations of current editor
				var decors = this.definedEditors.get(editor.document.uri.toString());
				// draw decorations of visible editor
				if (decors !== undefined) {
					// for each of its saved decorations, draw them
					decors.forEach((ranges: vscode.Range[], scope: string) => {
						editor.setDecorations(this.decorationTypes.get(scope).type, ranges);
					});
				}
			}
		});
	}

	updateColors() {
		// get colors from config
		const config = vscode.workspace.getConfiguration('hlasm');
		if (!config)
			return;
		const colors = config.get<{ id: string, hex: string }[]>('semanticHighlightingColors');
		if (colors != null) {
			// wipe all the decorations (clean)
			this.decorationTypes.forEach(type => {
				type.type.dispose();
			});
			// for each color, create its decoration
			for (let color of colors) {
				this.decorationTypes.set(color.id, new DecorationType(color.hex));
			}
		}
	}
}

class DecorationType {
	public type: vscode.TextEditorDecorationType;
	public hex: string;
	constructor(hex: string) {
		this.type = vscode.window.createTextEditorDecorationType({ color: hex });
		this.hex = hex;
	}
}
