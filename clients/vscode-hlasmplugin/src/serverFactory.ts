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

import * as vscodelc from 'vscode-languageclient';
import * as net from 'net';
import * as fork from 'child_process'
import * as path from 'path'

import { getConfig } from './eventsHandler'

/**
 * factory to create server options
 * also stores port used for DAP
 */
export class ServerFactory {
    private usedPorts: Set<number>;
    dapPort: number;
    lspPort: number;

    constructor() {
        this.usedPorts = new Set();
    }
    
    async create(useTcp: boolean) : Promise<vscodelc.ServerOptions> {
        const langServerFolder = process.platform;
        this.dapPort = await this.getPort();
        if (useTcp) {
            this.lspPort = await this.getPort();

            //spawn the server
            fork.spawnSync(
                path.join(__dirname, '..', 'bin', langServerFolder, 'language_server'),
                ["-p", this.dapPort.toString(), this.lspPort.toString()]);

            return () => {
                let socket = net.connect(this.lspPort,'localhost');
                let result: vscodelc.StreamInfo = {
                    writer: socket,
                    reader: socket
                };
                return Promise.resolve(result);
            };
        }
        else {
            const server: vscodelc.Executable = {
                command: path.join(__dirname, '..', 'bin', langServerFolder, 'language_server'),
                args: getConfig<string[]>('arguments').concat(["-p", this.dapPort.toString()])
            };
            return server;
        }
    }

    private async getPort() : Promise<number> {
        while (true) {
            const port = await this.getRandomPort();
            if (!this.usedPorts.has(port)) {
                this.usedPorts.add(port);
                return port;
            }
        }
    }
    // returns random free port
    private getRandomPort = () => new Promise<number>((resolve, reject) => {
        var srv = net.createServer();
        srv.unref();
        srv.listen(0, () => {
            const address = srv.address();
            srv.close(() => {
                resolve((address as net.AddressInfo).port);
            });
        });
    });
}

