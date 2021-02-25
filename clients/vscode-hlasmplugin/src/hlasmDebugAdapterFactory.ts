/*
 * Copyright (c) 2021 Broadcom.
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
import { BaseLanguageClient } from 'vscode-languageclient';

export class HLASMDebugAdapterFactory implements vscode.DebugAdapterDescriptorFactory {
    private client: BaseLanguageClient;
    constructor(client: BaseLanguageClient) {
        this.client = client;
    }
    createDebugAdapterDescriptor(session: vscode.DebugSession, executable: vscode.DebugAdapterExecutable): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
        return new vscode.DebugAdapterInlineImplementation(new HLASMDebugAdapter(this.client));
    }
    dispose() { }
}

class HLASMDebugAdapter implements vscode.DebugAdapter {
    private message_event = new vscode.EventEmitter<vscode.DebugProtocolMessage>();
    private static readonly message_id: string = "broadcom/hlasm/dsp_tunnel";

    constructor(private client: BaseLanguageClient) {
        this.client.onReady().then(() => {
            client.onNotification(HLASMDebugAdapter.message_id, (msg: any) => {
                this.message_event.fire(msg);
            });
        });
    }
    onDidSendMessage(listener: (e: vscode.DebugProtocolMessage) => any, thisArgs?: any, disposables?: vscode.Disposable[]): vscode.Disposable {
        return this.message_event.event(listener, thisArgs, disposables);
    }

    handleMessage(message: vscode.DebugProtocolMessage): void {
        this.client.sendNotification(HLASMDebugAdapter.message_id, message);
    }

    dispose() {
    }

}