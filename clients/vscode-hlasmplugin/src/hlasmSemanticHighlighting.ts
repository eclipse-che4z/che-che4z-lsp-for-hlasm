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

/**
 * Extended semantic highlighting feature
 * On top of the Semantic Highlighting, this provides continuation information retrieved from the server as part of the semantic highlighting request
 */

import { SemanticHighlightingFeature, ExtendedClientCapabilities, SemanticHighlightingParams } from './semanticHighlighting'
import { BaseLanguageClient } from 'vscode-languageclient';

declare class ContinuationInfo {
    continuationPositions: ContinuationPosition[];
    global: {
        continueColumn: number,
        continuationColumn: number
    }
}

declare class ContinuationPosition {
    line: number;
    continuationPosition: number;
}

export interface HLASMSemanticHighlightingParams extends SemanticHighlightingParams
{
    /**
	 * Information about continuation in current file
	 * Contains global continuation position, continue position (set by ICTL) and line-specific continuation positions (DBCS case) 
	 */
	continuation: ContinuationInfo;
}

/**
 * Capability similar to Semantic Highlighting
 */
export interface HLASMSemanticHighlightingClientCapabilities {
	textDocument?: {
		ASMsemanticHighlightingCapabilities?: {
			ASMsemanticHighlighting: boolean;
		}
	}
}

// export new client capabilities
export type HLASMExtendedClientCapabilities = ExtendedClientCapabilities & HLASMSemanticHighlightingClientCapabilities;

type line = number;
type column = number;


class DocumentContinuation {
	lineContinuations: Map<line,column>;
	continueColumn: column;
	continuationColumn: column;
}

/**
 * Feature itself adds new capabilities to highlighting
 */
export class HLASMSemanticHighlightingFeature extends SemanticHighlightingFeature
{
	protected documents: Map<string,DocumentContinuation>;
	constructor(client: BaseLanguageClient) {
        super(client);
		this.documents = new Map();
    }

	fillClientCapabilities(capabilities: HLASMExtendedClientCapabilities): void {
		if (!!capabilities.textDocument) {
			capabilities.textDocument = {};
		}
		
		capabilities.textDocument!.ASMsemanticHighlightingCapabilities = {
			ASMsemanticHighlighting: true
		};
	}

	/**
	 * Process continuations first and then apply decorations
	 * @param params 
	 */
    applyDecorations(params?: HLASMSemanticHighlightingParams | SemanticHighlightingParams): void {
		if (<HLASMSemanticHighlightingParams>params !== undefined)
        	this.processContInfo(<HLASMSemanticHighlightingParams>params);
        super.applyDecorations(params);
    }

	/**
	 * Saves document-specific continuation information
	 * @param params 
	 */
    private processContInfo(params: HLASMSemanticHighlightingParams)
    {
		if (params)
		{
			this.documents.set(params.textDocument.uri, {
				lineContinuations: new Map<line, column>(params.continuation.continuationPositions.map(x => [x.line, x.continuationPosition] as [line, column])),
				continueColumn: params.continuation.global.continueColumn,
				continuationColumn: params.continuation.global.continuationColumn
				});
		}
    }

	/**
	 * Returns continuation position for issued document/line
	 * @param line line to check continuation for, if -1 check general case
	 * @param uri document to check
	 * return continuation offset for document-line, -1 if there is none
	 */
    getContinuation(line: number, uri: string) {
		var foundDoc = this.documents.get(uri);
		//general case
		if (line == -1)
			return foundDoc.continuationColumn;
		return (foundDoc  && foundDoc.lineContinuations.get(line)) ? foundDoc.lineContinuations.get(line) : -1;
	}

	/**
	 * Returns continue column position for document
	 * @param uri document
	 */
	getContinueColumn(uri: string)
	{
		return this.documents.get(uri).continueColumn;
	}
}