/* --------------------------------------------------------------------------------------------
 * Copyright (c) TypeFox. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';

import { NotificationType } from 'vscode-jsonrpc';
import { VersionedTextDocumentIdentifier } from 'vscode-languageserver-types';

/**
 * Parameters for the semantic highlighting (server-side) push notification.
 */
export interface SemanticHighlightingParams {

	// First array represents continue column position and general continuation position, the rest are pairs of lines-continuation positions (DBCS case)
	continuationPositions: number[][];
	/**
	 * The text document that has to be decorated with the semantic highlighting information.
	 */
	textDocument: VersionedTextDocumentIdentifier;

	/**
	 * An array of semantic highlighting information.
	 */
	tokens: SemanticHighlightingInformation[];

}

/**
 * Represents a semantic highlighting information that has to be applied on a specific line of the text document.
 */
export interface SemanticHighlightingInformation {

	lineStart: number;
	columnStart: number;
	lineEnd: number;
	columnEnd: number;
	scope: string;
}

/**
 * Language server push notification providing the semantic highlighting information for a text document.
 */
export namespace SemanticHighlightingNotification {
	export const type = new NotificationType<SemanticHighlightingParams, void>('textDocument/semanticHighlighting');
}

/**
 * Capability that has to be set by the language client if that supports the semantic highlighting feature for the text documents.
 */
export interface SemanticHighlightingClientCapabilities {

	/**
	 * The text document client capabilities.
	 */
	textDocument?: {

		/**
		 * The client's semantic highlighting capability.
		 */
		semanticHighlightingCapabilities?: {

			/**
			 * `true` if the client supports semantic highlighting support text documents. Otherwise, `false`. It is `false` by default.
			 */
			semanticHighlighting: boolean;

		}

	}
}

/**
 * Semantic highlighting server capabilities.
 */
export interface SemanticHighlightingServerCapabilities {
	semanticHighlighting: boolean;
}
