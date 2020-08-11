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

//import { SemanticHighlightingFeature, ExtendedClientCapabilities, SemanticHighlightingParams, EditorColorsMap } from './semanticHighlighting'
import { BaseLanguageClient } from 'vscode-languageclient';

export class ContinuationInfo {
    continuationPositions: ContinuationPosition[];
    global: {
        continueColumn: number,
        continuationColumn: number
    }
}

export class ContinuationPosition {
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



export type ContinuationDocumentsInfo = Map<string,DocumentContinuation>;

export class DocumentContinuation {
	constructor() {
		this.lineContinuations = new Map();
		this.continuationColumn = -1;
		this.continueColumn = -1;
	}
	lineContinuations: Map<line,column>;
	continueColumn: column;
	continuationColumn: column;
}

/**
 * Feature itself adds new capabilities to highlighting
 */
export class HLASMSemanticHighlightingFeature extends SemanticHighlightingFeature
{
	protected documents: ContinuationDocumentsInfo;
	private parseFinishedCallback: (parsedFile: string) => void;
	constructor(client: BaseLanguageClient, parseFinishedCallback?: (parsedFile: string) => void) {
        super(client);
		this.documents = new Map();
		this.parseFinishedCallback = parseFinishedCallback;
    }

	registerParseFinishedCallback(parseFinishedCallback: (parsedFile: string) => void) {
		this.parseFinishedCallback = parseFinishedCallback;
	}

	fillClientCapabilities(capabilities: HLASMExtendedClientCapabilities): void {
		if (!!capabilities.textDocument) {
			capabilities.textDocument = {};
		}
		
		capabilities.textDocument.ASMsemanticHighlightingCapabilities = {
			ASMsemanticHighlighting: true
		};
	}

	/**
	 * Process continuations first and then apply decorations
	 * @param params 
	 */
    applyDecorations(params?: HLASMSemanticHighlightingParams | SemanticHighlightingParams): EditorColorsMap {
		if (this.parseFinishedCallback)
			this.parseFinishedCallback(params.textDocument.uri);

		if ((params as HLASMSemanticHighlightingParams).continuation !== undefined)
        	this.processContInfo(params as HLASMSemanticHighlightingParams);
		return super.applyDecorations(params);
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

	getContinuationInfo(): ContinuationDocumentsInfo {
		return this.documents;
	}
}
