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

export function HLASMExternalFilesFtp(context: vscode.ExtensionContext): ClientInterface<ConnectionInfo, DatasetUriDetails, DatasetUriDetails> {
    return {
        getConnInfo: async () => {
            const last = getLastRunConfig(context);
            const info = await gatherConnectionInfo(last);
            await updateLastRunConfig(context, { host: info.host, user: info.user, jobcard: last.jobcard });

            return { info, uniqueId: info.host + ':' + (info.port ?? '21') };
        },
        parseArgs: (path: string, purpose: ExternalRequestType) => {
            const [dataset, member] = path.split('/').slice(1).map(x => x.toUpperCase());

            if (!dataset || !/^(?:[A-Z$#@][A-Z$#@0-9]{1,7})(?:\.(?:[A-Z$#@][A-Z$#@0-9]{1,7}))*$/.test(dataset) || dataset.length > 44) return null;
            switch (purpose) {
                case ExternalRequestType.read_file:
                    if (!member || !/^[A-Z$#@][A-Z$#@0-9]{1,7}(?:\..*)?$/.test(member)) return null; // ignore extension
                    break;
                case ExternalRequestType.read_directory:
                    if (member) return null;
                    break;
            }

            return new DatasetUriDetails(dataset, member?.split('.', 1)[0] || null);
        },
        createClient: () => {
            const client = new ftp.Client();
            client.parseList = (rawList: string): ftp.FileInfo[] => {
                return rawList.split(/\r?\n/).slice(1).filter(x => !/^\s*$/.test(x)).map(value => new ftp.FileInfo(value.split(/\s/, 1)[0]));
            };
            return {
                dispose: () => { client.close(); },

                connect: async (arg: ConnectionInfo) => { await client.access(translateConnectionInfo(arg)) },

                listMembers: async (args: DatasetUriDetails): Promise<string[] | null> => {
                    try {
                        await checkedCommand(client, 'TYPE A');
                        checkResponse(await client.cd(`'${args.dataset}'`));
                        const list = await client.list();
                        return list.map(x => x.name);
                    }
                    catch (e) {
                        if (e instanceof ftp.FTPError && e.code == 550)
                            return null;
                        throw e;
                    }
                },

                readMember: async (args: DatasetUriDetails): Promise<string | null> => {
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
                },

                reusable: () => !client.closed,
            }
        }
    }
}
