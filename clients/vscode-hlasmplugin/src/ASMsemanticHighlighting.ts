import { SemanticHighlightingFeature, ExtendedClientCapabilities, SemanticHighlightingParams } from './semanticHighlighting'
import { BaseLanguageClient } from 'vscode-languageclient';
import { stringify } from 'querystring';

export interface ASMSemanticHighlightingParams extends SemanticHighlightingParams
{
    /**
	 * Information about continuation in current file
	 * Contains global continuation position, continue position (set by ICTL) and line-specific continuation positions (DBCS case) 
	 */
	continuation: continuationInfo;
}

/**
 * Capability that has to be set by the language client if that supports the semantic highlighting feature for the text documents.
 */
export interface ASMSemanticHighlightingClientCapabilities {

	/**
	 * The text document client capabilities.
	 */
	textDocument?: {

		/**
		 * The client's semantic highlighting capability.
		 */
		ASMsemanticHighlightingCapabilities?: {

			/**
			 * `true` if the client supports semantic highlighting support text documents. Otherwise, `false`. It is `false` by default.
			 */
			ASMsemanticHighlighting: boolean;
		}
	}
}

export type ASMExtendedClientCapabilities = ExtendedClientCapabilities & ASMSemanticHighlightingClientCapabilities;

type line = number;
type column = number;

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

    public applyDecorations(params?: ASMSemanticHighlightingParams | SemanticHighlightingParams): void {
		if (<ASMSemanticHighlightingParams>params !== undefined)
        	this.processContInfo(<ASMSemanticHighlightingParams>params);
        super.applyDecorations(params);
    }

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

    public getContinuation(line: number, uri: string) {
		var foundDoc = this.documents.get(uri);
		if (line == -1)
			return foundDoc.continuationColumn;
		return (foundDoc  && foundDoc.lineContinuations.get(line)) ? foundDoc.lineContinuations.get(line) : -1;
	}
	public getContinueColumn(uri: string)
	{
		return this.documents.get(uri).continueColumn;
	}
}

class DocumentContinuation {
	lineContinuations: Map<line,column>;
	continueColumn: column;
	continuationColumn: column;
}

declare class continuationInfo {
    continuationPositions: continuationPosition[];
    global: {
        continueColumn: number,
        continuationColumn: number
    }
}

declare class continuationPosition {
    line: number;
    continuationPosition: number;
}