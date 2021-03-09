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
import * as net from 'net'
import { BaseLanguageClient } from 'vscode-languageclient';

export class HLASMDebugAdapterFactory implements vscode.DebugAdapterDescriptorFactory {
    private theia_local_server: net.Server = null;
    private theia_local_port: number = 0;

    constructor(private client: BaseLanguageClient) {
        if (typeof (vscode.DebugAdapterInlineImplementation) === 'undefined')
            this.setup_theia_compatibility_server();
    }
    createDebugAdapterDescriptor(session: vscode.DebugSession, executable: vscode.DebugAdapterExecutable): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
        if (this.theia_local_server)
            return new vscode.DebugAdapterServer(this.theia_local_port, '127.0.0.1');
        else
            return new vscode.DebugAdapterInlineImplementation(new HLASMDebugAdapter(this.client));
    }

    dispose() {
        if (this.theia_local_server)
            this.theia_local_server.close();
    }

    // This creates a simple proxy for Theia
    // and can be removed as soon as DebugAdapterInlineImplementation is supported
    setup_theia_compatibility_server() {
        const me = this;
        const content_length = 'Content-Length: ';

        const server = net.createServer();
        this.theia_local_server = server;
        this.theia_local_server.on('connection', function (socket: net.Socket) {
            const hlasm_client = new HLASMDebugAdapter(me.client);
            let buffer = Buffer.from([]);

            hlasm_client.onDidSendMessage(function (msg: vscode.DebugProtocolMessage) {
                const msg_buffer = Buffer.from(JSON.stringify(msg));
                socket.write(content_length);
                socket.write(msg_buffer.length.toString());
                socket.write('\r\n\r\n');
                socket.write(msg_buffer);
            });
            socket.on('data', function (data: Buffer) {
                buffer = Buffer.concat([buffer, data]);
                while (true) {
                    if (buffer.indexOf(content_length) != 0)
                        return;
                    const end_of_line = buffer.indexOf('\r\n');
                    if (end_of_line < 0)
                        return;
                    const length = +buffer.slice(content_length.length, end_of_line);
                    const end_of_headers = buffer.indexOf('\r\n\r\n');
                    if (end_of_headers < 0)
                        return;
                    const data_start = end_of_headers + 4;
                    const data_end = data_start + length;
                    if (data_end > buffer.length)
                        return;
                    const json = JSON.parse(buffer.slice(data_start, data_end).toString());
                    hlasm_client.handleMessage(json);
                    buffer = buffer.slice(data_end);
                }
            });
            socket.on('close', function () {
                hlasm_client.dispose();
            });
        });
        this.theia_local_server.listen(0, '127.0.0.1', function () {
            me.theia_local_port = (<net.AddressInfo>server.address()).port;
        });
    }
}

class HLASMDebugAdapter implements vscode.DebugAdapter {
    private static next_session_id: number = 0;
    private static readonly registration_message_id: string = 'broadcom/hlasm/dap_tunnel';

    private message_event = new vscode.EventEmitter<vscode.DebugProtocolMessage>();
    private readonly session_id: number = HLASMDebugAdapter.next_session_id++;
    private readonly message_id: string = HLASMDebugAdapter.registration_message_id + '/' + this.session_id;

    constructor(private client: BaseLanguageClient) {
        this.client.sendNotification(HLASMDebugAdapter.registration_message_id, this.session_id);
        this.client.onReady().then(() => {
            client.onNotification(this.message_id, (msg: any) => {
                this.message_event.fire(msg);
            });
        });
    }
    onDidSendMessage(listener: (e: vscode.DebugProtocolMessage) => any, thisArgs?: any, disposables?: vscode.Disposable[]): vscode.Disposable {
        return this.message_event.event(listener, thisArgs, disposables);
    }

    handleMessage(message: vscode.DebugProtocolMessage): void {
        this.client.sendNotification(this.message_id, message);
    }

    dispose() {
        this.client.sendNotification(this.message_id);
        this.message_event.dispose();
    }

}
