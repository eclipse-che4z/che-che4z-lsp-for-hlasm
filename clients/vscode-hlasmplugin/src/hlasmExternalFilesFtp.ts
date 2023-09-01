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
import * as ftp from 'basic-ftp';
import { ClientInterface, ClientUriDetails, ExternalRequestType } from './hlasmExternalFiles';
import { ConnectionInfo, gatherConnectionInfo, getLastRunConfig, translateConnectionInfo, updateLastRunConfig } from './ftpCreds';
import { FBWritable } from './FBWritable';
import { ConnectionPool } from './connectionPool';
import { AsyncMutex } from './asyncMutex';

const checkResponse = (resp: ftp.FTPResponse) => {
    if (resp.code < 200 || resp.code > 299) throw Error("FTP Error: " + resp.message);
    return resp;
}

const checkedCommand = async (client: ftp.Client, command: string): Promise<string> => {
    return checkResponse(await client.send(command)).message
}

class DatasetUriDetails implements ClientUriDetails {
    constructor(
        public readonly dataset: string,
        public readonly member: string | null) { }

    toDisplayString() {
        if (this.member)
            return `${this.dataset}(${this.member})`;
        else
            return this.dataset;
    }

    normalizedPath() {
        return `/${this.dataset}/${this.member ?? ''}`;
    }
};

const connectionPoolSize = 4;
const connectionPoolTimeout = 30000;

export function HLASMExternalFilesFtp(context: vscode.ExtensionContext): ClientInterface<undefined, DatasetUriDetails, DatasetUriDetails> {
    let activeConnectionInfo: ConnectionInfo | undefined = undefined;

    const mutex = new AsyncMutex();
    const getConnInfo = async () => {
        const last = getLastRunConfig(context);
        const info = await gatherConnectionInfo(last);
        await updateLastRunConfig(context, { host: info.host, user: info.user, jobcard: last.jobcard });

        return info;
    };
    let allowCredQuery = 0;
    let mirrorCredQuery = allowCredQuery;

    const pool = new ConnectionPool<ftp.Client>({
        create: async () => {
            const client = new ftp.Client();
            client.parseList = (rawList: string): ftp.FileInfo[] => {
                return rawList.split(/\r?\n/).slice(1).filter(x => !/^\s*$/.test(x)).map(value => new ftp.FileInfo(value.split(/\s/, 1)[0]));
            };
            return mutex.locked(async () => {
                const info = activeConnectionInfo ?? (pool.closeClients(), await getConnInfo());
                activeConnectionInfo = undefined;
                await client.access(translateConnectionInfo(info));
                activeConnectionInfo = info;

                return client;
            });
        },
        reusable: (client) => !client.closed,
        close: (client) => { try { client.close(); } catch (e) { } },
    }, connectionPoolSize, connectionPoolTimeout);

    return {
        parseArgs: async (path: string, purpose: ExternalRequestType) => {
            const [dataset, member] = path.split('/').slice(1).map(decodeURIComponent).map(x => x.toUpperCase());

            if (!dataset || !/^(?:[A-Z$#@][A-Z$#@0-9]{1,7})(?:\.(?:[A-Z$#@][A-Z$#@0-9]{1,7}))*$/.test(dataset) || dataset.length > 44) return null;
            switch (purpose) {
                case ExternalRequestType.read_file:
                    if (!member || !/^[A-Z$#@][A-Z$#@0-9]{1,7}(?:\..*)?$/.test(member)) return null; // ignore extension
                    break;
                case ExternalRequestType.list_directory:
                    if (member) return null;
                    break;
            }

            return { details: new DatasetUriDetails(dataset, member?.split('.', 1)[0] || null), server: undefined };
        },

        listMembers: async (args: DatasetUriDetails): Promise<string[] | null> => pool.withClient(async (client) => {
            try {
                await checkedCommand(client, 'TYPE A');
                checkResponse(await client.cd(`'${args.dataset}'`));
                const list = await client.list();
                return list.map(x => `/${args.dataset}/${x.name}.hlasm`);
            }
            catch (e) {
                if (e instanceof ftp.FTPError && e.code == 550)
                    return null;
                throw e;
            }
        }),

        readMember: async (args: DatasetUriDetails): Promise<string | null> => pool.withClient(async (client) => {
            try {
                const buffer = new FBWritable();
                buffer.on('error', err => { throw err });

                await checkedCommand(client, 'TYPE I');
                checkResponse(await client.downloadTo(buffer, `'${args.dataset}(${args.member!})'`));

                return buffer.getResult();
            }
            catch (e) {
                if (e instanceof ftp.FTPError && e.code == 550)
                    return null;
                throw e;
            }
        }),

        serverId: () => mutex.locked(async () => {
            const transform = (arg: ConnectionInfo) => arg.host + ':' + (arg.port ?? '21');

            if (activeConnectionInfo)
                return transform(activeConnectionInfo);

            if (mirrorCredQuery !== allowCredQuery)
                return undefined;

            ++allowCredQuery;
            activeConnectionInfo = await getConnInfo();
            mirrorCredQuery = allowCredQuery;

            return transform(activeConnectionInfo);
        }),

        suspended: () => { ++allowCredQuery; pool.closeClients(); },
        resumed: () => { mirrorCredQuery = allowCredQuery; },
        dispose: () => pool.closeClients(),
    };
}
