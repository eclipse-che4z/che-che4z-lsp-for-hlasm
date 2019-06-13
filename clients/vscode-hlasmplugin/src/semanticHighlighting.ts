/* --------------------------------------------------------------------------------------------
 * Copyright (c) TypeFox. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';

import { Disposable, Uri, Range, window, DecorationRenderOptions, TextEditorDecorationType, workspace, TextEditor, Position } from 'vscode';
import {
	TextDocumentRegistrationOptions, ClientCapabilities, ServerCapabilities, DocumentSelector, NotificationHandler,
} from 'vscode-languageserver-protocol';

import {SemanticHighlightingNotification, SemanticHighlightingParams, SemanticHighlightingInformation, SemanticHighlightingClientCapabilities} from './protocol.semanticHighlighting'

import * as UUID from '../node_modules/vscode-languageclient/lib/utils/uuid';
import { TextDocumentFeature, BaseLanguageClient, LanguageClient } from 'vscode-languageclient';

export declare type ExtendedClientCapabilities = ClientCapabilities & SemanticHighlightingClientCapabilities;

export class ExtendedLanguageClient extends LanguageClient
{
	semanticHighlighting: SemanticHighlightingFeature;
	protected registerBuiltinFeatures() : void
	{
		var semanticHighlighting = new SemanticHighlightingFeature(this);
		super.registerBuiltinFeatures();
		this.registerFeature(semanticHighlighting);
		this.semanticHighlighting = semanticHighlighting;
	}

	public applyDecorations(){
		this.semanticHighlighting.applyDecorations();
	}
}

export { SemanticHighlightingParams }

export * from 'vscode-languageclient'

type scope = string;
type uri = string;

export class SemanticHighlightingFeature extends TextDocumentFeature<TextDocumentRegistrationOptions> {

	protected readonly toDispose: Disposable[];
	protected readonly handlers: NotificationHandler<SemanticHighlightingParams>[];
	// map of possible decoration types: scope of symbol -> color of scope
	protected decorationTypes: Map<scope,decorationType>;
	// map: document -> its decorations (map: scope of symbol -> ranges of scope)
	protected definedEditors: Map<uri,Map<scope,Range[]>>;
	constructor(client: BaseLanguageClient) {
		super(client, SemanticHighlightingNotification.type);
		this.toDispose = [];
		this.handlers = [];
		this.decorationTypes = new Map();
		this.definedEditors = new Map();
		this.toDispose.push({ dispose: () => this.definedEditors.clear() });
		this.toDispose.push(workspace.onDidCloseTextDocument(e => {
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
		// update colors from config
		this.updateColors();
		// for each visible editor
		window.visibleTextEditors.forEach(editor => {
			// find the decorations of current editor
			var decors = this.definedEditors.get(editor.document.uri.toString());
			// check whether current editor matches editor from params
			if (params && params.textDocument.uri == editor.document.uri.toString()) {
				// editor found, clear its decorations
				if (decors == undefined) {
					this.definedEditors.set(params.textDocument.uri, new Map<scope,Range[]>());
					decors = this.definedEditors.get(params.textDocument.uri);
				}
				// clear ranges
				Array.from(this.decorationTypes.keys()).forEach(scope => {
					decors.set(scope,[]);
				})
				// add range for each token to its corresponding scope
				params.tokens.forEach(token => {
					decors.get(token.scope)!.push(new Range(new Position(token.lineStart, token.columnStart), new Position(token.lineEnd, token.columnEnd)));
				});
			}
			
			// draw decorations of visible editor (whether it is in params or not)
			if (decors !== undefined) {
				// for each of its saved decorations, draw them
				decors.forEach((ranges: Range[], scope: string) => {
					editor.setDecorations(this.decorationTypes.get(scope).type,ranges);
				});
			}
		});
	}

    public updateColors() {
		// get colors from config
		var colors = workspace.getConfiguration().semanticHighlightingColors;
		if (colors != null) {
			// wipe all the decorations (clean)
			this.decorationTypes.forEach(type => {
				type.type.dispose();
			});
			// for each color, create its decoration
			for (let color of colors) {
				this.decorationTypes.set(color.id,new decorationType(color.hex));
			}
		}
    }
}

class decorationType {
	public type: TextEditorDecorationType;
	public hex: string;
    constructor(hex:string)
    {
		this.type = window.createTextEditorDecorationType({color:hex});
		this.hex = hex;
	}
}