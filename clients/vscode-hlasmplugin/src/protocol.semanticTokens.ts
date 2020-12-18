/* --------------------------------------------------------------------------------------------
 * Copyright (c) TypeFox. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';

import * as vscode from 'vscode';
import { RequestType } from 'vscode-jsonrpc';
import { TextDocumentRegistrationOptions } from 'vscode-languageclient';

/**
 * Modified interfaces taken from the LSP specification of the upcoming 3.16 version.
 * Once released, all this can be removed
 */

/**
 * Parameters for the semantic highlighting (server-side)
 */
export interface SemanticTokensParams {
	textDocument: {
		uri: string;
	}
}

/**
 * Result for the semantic highlighting (server-side)
 */
export interface SemanticTokensResult {
	/**
	 * An array of semantic highlighting information.
	 */
	data: number[];
}

export interface SemanticTokensServerCapabilities {
	semanticTokensProvider : SemanticTokensOptions;
}

export interface SemanticTokensOptions {
	/**
	 * The legend used by the server
	 */
	legend: vscode.SemanticTokensLegend;
}

export interface SemanticTokensRegistrationOptions extends TextDocumentRegistrationOptions, SemanticTokensOptions  {
}

/**
 * Client to server request for semantic tokens
 */
export namespace SemanticTokensRequest {
	export const type = new RequestType<SemanticTokensParams, SemanticTokensResult, void, SemanticTokensRegistrationOptions>('textDocument/semanticTokens/full');
}
