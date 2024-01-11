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

export type AsmOptions = (
    {
        GOFF: boolean;
    } | {
        XOBJECT: boolean;
    }
    | {}
) & ({
    MACHINE:
    | "ARCH-0"
    | "ARCH-1"
    | "ARCH-10"
    | "ARCH-11"
    | "ARCH-12"
    | "ARCH-13"
    | "ARCH-14"
    | "ARCH-2"
    | "ARCH-3"
    | "ARCH-4"
    | "ARCH-5"
    | "ARCH-6"
    | "ARCH-7"
    | "ARCH-8"
    | "ARCH-9"
    | "S370"
    | "S370ESA"
    | "S370XA"
    | "S390"
    | "S390E"
    | "ZS"
    | "ZS-1"
    | "ZS-2"
    | "ZS-3"
    | "ZS-4"
    | "ZS-5"
    | "ZS-6"
    | "ZS-7"
    | "ZS-8"
    | "ZS-9"
    | "ZS-10"
    | "z10"
    | "z11"
    | "z114"
    | "z12"
    | "z13"
    | "z14"
    | "z15"
    | "z16"
    | "z196"
    | "z800"
    | "z890"
    | "z9"
    | "z900"
    | "z990"
    | "zBC12"
    | "zEC12"
    | "zSeries"
    | "zSeries-1"
    | "zSeries-2"
    | "zSeries-3"
    | "zSeries-4"
    | "zSeries-5"
    | "zSeries-6"
    | "zSeries-7"
    | "zSeries-8"
    | "zSeries-9"
    | "zSeries-10";
} | {
    OPTABLE:
    | "UNI"
    | "DOS"
    | "370"
    | "XA"
    | "ESA"
    | "ZOP"
    | "ZS1"
    | "YOP"
    | "ZS2"
    | "Z9"
    | "ZS3"
    | "Z10"
    | "ZS4"
    | "Z11"
    | "ZS5"
    | "Z12"
    | "ZS6"
    | "Z13"
    | "ZS7"
    | "Z14"
    | "ZS8"
    | "Z15"
    | "ZS9"
    | "Z16"
    | "ZSA";
}
    | {}
    ) & {
        SYSPARM?: string;
        PROFILE?: string;
        SYSTEM_ID?: string;
        RENT?: boolean;
    };

export type Preprocessor = ("DB2" | "CICS" | "ENDEVOR")
    | {
        name: "DB2";
        options?: {
            version?: string;
            conditional?: boolean;
        };
    }
    | {
        name: "CICS";
        options?: string[];
    }
    | {
        name: "ENDEVOR";
    };

export type Library = string
    | {
        path: string;
        optional?: boolean;
        macro_extensions?: string[];
        prefer_alternate_root?: boolean;
    } | {
        dataset: string;
        optional?: boolean;
        profile?: string;
    } | {

        environment: string;
        stage: string;
        system: string;
        subsystem: string;
        type: string;
        use_map?: boolean;
        optional?: boolean;
        profile?: string;
    };

/**
 * Derived from schema/processor_entry.schema.json
 */
export interface ProcGrpsSchemaJson {
    name: string;
    libs: Library[];
    asm_options?: AsmOptions;
    preprocessor?: Preprocessor | Preprocessor[];
};

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

export interface ConfigurationProviderRegistration {
    dispose(): void;
    invalidate(uri: vscode.Uri | null): PromiseLike<void> | void;
};

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
        this.channel.sendNotification('invalidate_external_configuration', uri ? { uri: uri.toString() } : {});
    }

    public addHandler(h: HLASMExternalConfigurationProviderHandler): ConfigurationProviderRegistration {
        this.requestHandlers.push(h);
        this.invalidateConfiguration(null);

        return Object.freeze({
            dispose: () => {
                const idx = this.requestHandlers.indexOf(h);
                if (idx >= 0)
                    this.requestHandlers.splice(idx, 1);
                this.invalidateConfiguration(null);
            },
            invalidate: (uri: vscode.Uri | null) => { this.invalidateConfiguration(uri); }
        });
    }
}
