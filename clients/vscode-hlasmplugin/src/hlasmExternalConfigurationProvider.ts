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

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient';

interface ExternalConfigurationRequest {
    uri: string;
};
interface ProcGrpsSchemaJson { };
interface ExternalConfigurationResponse {
    configuration: string | ProcGrpsSchemaJson;
};

export type HLASMExternalConfigurationProviderHandler = (uri: vscode.Uri) => PromiseLike<ExternalConfigurationResponse | null> | ExternalConfigurationResponse | null;

function isExternalConfigurationRequest(p: any): p is ExternalConfigurationRequest {
    return typeof p === 'object' && ('uri' in p);
}

function isError(e: any): e is Error {
    return e instanceof Error;
}

export class HLASMExternalConfigurationProvider {
    private toDispose: vscode.Disposable[] = [];
    private requestHandlers: HLASMExternalConfigurationProviderHandler[] = [];

    constructor(
        private channel: {
            onRequest<R, E>(method: string, handler: vscodelc.GenericRequestHandler<R, E>): vscode.Disposable;
            sendNotification(method: string, params: any): Promise<void>;
        }) {
        this.toDispose.push(this.channel.onRequest('external_configuration_request', (...params: any[]) => this.handleRawMessage(...params)));
    }

    dispose() {
        this.toDispose.forEach(x => x.dispose());
    }

    private async handleRequest(uri: vscode.Uri): Promise<ExternalConfigurationResponse | vscodelc.ResponseError> {
        for (const h of this.requestHandlers) {
            try {
                const resp = await h(uri);
                if (resp)
                    return resp;
            }
            catch (e) {
                return new vscodelc.ResponseError(-106, isError(e) ? e.message : 'Unknown error');
            }
        }

        return new vscodelc.ResponseError(0, 'Not found');
    }

    public async handleRawMessage(...params: any[]): Promise<ExternalConfigurationResponse | vscodelc.ResponseError> {
        if (params.length < 1 || !isExternalConfigurationRequest(params[0]))
            return new vscodelc.ResponseError(-5, 'Invalid request');

        try {
            const uri = vscode.Uri.parse(params[0].uri);
            return this.handleRequest(uri);
        } catch (e) {
            return new vscodelc.ResponseError(-5, 'Invalid request');
        }
    }

    private invalidateConfiguration(uri: vscode.Uri | null) {
        return this.channel.sendNotification('invalidate_external_configuration', uri ? { uri: uri.toString() } : {});
    }

    public addHandler(h: HLASMExternalConfigurationProviderHandler) {
        this.requestHandlers.push(h);

        return {
            dispose: () => {
                const idx = this.requestHandlers.indexOf(h);
                if (idx >= 0)
                    this.requestHandlers.splice(idx, 1);
                return this.invalidateConfiguration(null);
            },
            invalidate: (uri: vscode.Uri | null) => this.invalidateConfiguration(uri)
        };
    }
}
