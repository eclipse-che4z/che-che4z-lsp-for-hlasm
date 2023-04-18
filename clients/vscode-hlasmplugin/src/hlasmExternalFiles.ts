/*
 * Copyright (c) 2023 Broadcom.
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

import * as vscode from "vscode";
import * as vscodelc from "vscode-languageclient";

// This is a temporary demo implementation

enum ExternalRequestType {
    read_file = 'read_file',
    read_directory = 'read_directory',
}

interface ExternalRequest {
    id: number,
    op: ExternalRequestType,
    url: string,
}

interface ExternalReadFileResponse {
    id: number,
    data: string,
}
interface ExternalReadDirectoryResponse {
    id: number,
    data: string[],
}
interface ExternalErrorResponse {
    id: number,
    error: {
        code: number,
        msg: string,
    },
}

function generateFileContent(uriPath: string) {
    return `.*
         MACRO
         ${uriPath.substring(uriPath.length - 4)}
         MEND`
}

const magicScheme = 'hlasm-external';
export class HLASMExternalFiles {
    private toDispose: vscode.Disposable[] = [];

    constructor(client: vscodelc.BaseLanguageClient) {
        this.toDispose.push(client.onNotification('external_file_request', params => this.handleRawMessage(params).then(
            msg => { if (msg) client.sendNotification('external_file_response', msg); }
        )));

        client.onDidChangeState(e => {
            if (e.newState === vscodelc.State.Starting)
                this.reset();
        }, this, this.toDispose);

        this.toDispose.push(vscode.workspace.registerTextDocumentContentProvider(magicScheme, {
            provideTextDocumentContent(uri: vscode.Uri, token: vscode.CancellationToken): vscode.ProviderResult<string> {
                if (uri.scheme === magicScheme && /\/MAC[A-C]$/.test(uri.path))
                    return generateFileContent(uri.path);
                return null;
            }
        }));
    }

    reset() { /* drop all pending requests in the future*/ }

    dispose() {
        this.toDispose.forEach(x => x.dispose());
    }

    private handleFileMessage(msg: ExternalRequest): Promise<ExternalReadFileResponse | ExternalReadDirectoryResponse | ExternalErrorResponse> {
        if (msg.url.startsWith(magicScheme) && /\/MAC[A-C]$/.test(msg.url))
            return Promise.resolve({
                id: msg.id,
                data: generateFileContent(msg.url)
            });
        else
            return Promise.resolve({
                id: msg.id,
                error: { code: 0, msg: 'Not found' }
            });
    }
    private handleDirMessage(msg: ExternalRequest): Promise<ExternalReadFileResponse | ExternalReadDirectoryResponse | ExternalErrorResponse> {
        if (msg.url.startsWith(magicScheme))
            return Promise.resolve({
                id: msg.id,
                data: ['MACA', 'MACB', 'MACC', 'MACD']
            });
        else
            return Promise.resolve({
                id: msg.id,
                error: { code: 0, msg: 'Not found' }
            });
    }

    public handleRawMessage(msg: any): Promise<ExternalReadFileResponse | ExternalReadDirectoryResponse | ExternalErrorResponse | null> {
        if (!msg || typeof msg.id !== 'number' || typeof msg.op !== 'string')
            return Promise.resolve(null);

        if (msg.op === ExternalRequestType.read_file && typeof msg.url === 'string')
            return this.handleFileMessage(msg);
        if (msg.op === ExternalRequestType.read_directory && typeof msg.url === 'string')
            return this.handleDirMessage(msg);

        return Promise.resolve({ id: msg.id, error: { code: -5, msg: 'Invalid request' } });
    }

}

