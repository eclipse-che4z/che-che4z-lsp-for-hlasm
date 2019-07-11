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

export interface ASMSemanticHighlightingParams extends SemanticHighlightingParams
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
export interface ASMSemanticHighlightingClientCapabilities {
	textDocument?: {
		ASMsemanticHighlightingCapabilities?: {
			ASMsemanticHighlighting: boolean;
		}
	}
}

// export new client capabilities
export type ASMExtendedClientCapabilities = ExtendedClientCapabilities & ASMSemanticHighlightingClientCapabilities;

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
export class ASMSemanticHighlightingFeature extends SemanticHighlightingFeature
{
	protected documents: Map<string,DocumentContinuation>;
	constructor(client: BaseLanguageClient) {
        super(client);
		this.documents = new Map();
    }

	fillClientCapabilities(capabilities: ASMExtendedClientCapabilities): void {
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
    public applyDecorations(params?: ASMSemanticHighlightingParams | SemanticHighlightingParams): void {
		if (<ASMSemanticHighlightingParams>params !== undefined)
        	this.processContInfo(<ASMSemanticHighlightingParams>params);
        super.applyDecorations(params);
    }

	/**
	 * Saves document-specific continuation information
	 * @param params 
	 */
    private processContInfo(params: ASMSemanticHighlightingParams)
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
    public getContinuation(line: number, uri: string) {
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
	public getContinueColumn(uri: string)
	{
		return this.documents.get(uri).continueColumn;
	}
}