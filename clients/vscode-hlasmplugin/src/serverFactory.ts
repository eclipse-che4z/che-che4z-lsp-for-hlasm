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

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';
import * as net from 'net';
import * as cp from 'child_process';
import * as path from 'path';
import { getConfig } from './eventsHandler';
import { decorateArgs, ServerVariant } from './serverFactory.common';

const supportedNativePlatforms: Readonly<{ [key: string]: string }> = Object.freeze({
    'win32.x64': 'win32',
    'linux.x64': 'linux',
    'darwin.x64': 'darwin_x64',
    'darwin.arm64': 'darwin_arm64',
});

export async function createLanguageServer(serverVariant: ServerVariant, clientOptions: vscodelc.LanguageClientOptions, extUri: vscode.Uri): Promise<vscodelc.BaseLanguageClient> {
    const serverOptions = await generateServerOption(serverVariant);

    return new vscodelc.LanguageClient('HLASM extension Language Server', serverOptions, clientOptions);
}

async function generateServerOption(method: ServerVariant): Promise<vscodelc.ServerOptions> {
    const langServerFolder = supportedNativePlatforms[process.platform + '.' + process.arch];
    if (!langServerFolder) {
        if (method !== 'wasm')
            console.log('Unsupported platform detected, switching to wasm');
        method = 'wasm';
    }
    if (method === 'tcp') {
        const lspPort = await getPort();

        //spawn the server
        cp.execFile(
            path.join(__dirname, '..', 'bin', langServerFolder, 'language_server'),
            decorateArgs([lspPort.toString()]));

        return () => {
            let socket = net.connect(lspPort, 'localhost');
            let result: vscodelc.StreamInfo = {
                writer: socket,
                reader: socket
            };
            return Promise.resolve(result);
        };
    }
    else if (method === 'native') {
        const server: vscodelc.Executable = {
            command: path.join(__dirname, '..', 'bin', langServerFolder, 'language_server'),
            args: decorateArgs(getConfig<string[]>('arguments', []))
        };
        return server;
    }
    else if (method === 'wasm') {
        const server: vscodelc.NodeModule = {
            module: path.join(__dirname, '..', 'bin', 'wasm', 'language_server'),
            args: decorateArgs(getConfig<string[]>('arguments', [])),
            options: { execArgv: getWasmRuntimeArgs() }
        };
        return server;
    }
    else {
        throw Error("Invalid method");
    }
}

function getWasmRuntimeArgs(): Array<string> {
    const v8Version = process?.versions?.v8 ?? "1.0";
    const v8Major = +v8Version.split(".")[0];
    if (v8Major >= 10)
        return [];
    else if (v8Major >= 9)
        return [
            '--experimental-wasm-eh',
        ];
    else
        return [
            '--experimental-wasm-threads',
            '--experimental-wasm-bulk-memory',
            '--experimental-wasm-eh',
        ];
}

async function getPort(): Promise<number> {
    const usedPorts = new Set<number>;
    while (true) {
        const port = await getRandomPort();
        if (!usedPorts.has(port)) {
            usedPorts.add(port);
            return port;
        }
    }
}
// returns random free port
const getRandomPort = () => new Promise<number>((resolve, reject) => {
    const srv = net.createServer();
    srv.unref();
    srv.listen(0, "127.0.0.1", () => {
        const address = srv.address();
        srv.close(() => {
            resolve((address as net.AddressInfo).port);
        });
    });
});
