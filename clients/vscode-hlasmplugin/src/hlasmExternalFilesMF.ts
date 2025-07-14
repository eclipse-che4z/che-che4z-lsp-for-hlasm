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
import { ClientInterface, ClientUriDetails, ExternalRequestType, SuspendError } from './hlasmExternalFiles';
import { FtpConnectionInfo, ZoweConnectionInfo, ensureValidMfZoweClient, gatherConnectionInfo, getLastRunConfig, translateConnectionInfo, updateLastRunConfig } from './mfCreds';
import { FBWritable } from './FBWritable';
import { ConnectionPool } from './connectionPool';
import { AsyncMutex } from './asyncMutex';
import { Writable } from 'stream';
import { concat, isCancellationError } from './helpers';
import { textDecode } from './tools.common';

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

type MFClient = {
    list(dataset: string): Promise<string[] | null>;
    read(dataset: string, member: string): Promise<string | null>;
    close(): void;
    closed: boolean;
}

function getZoweErrorCode(e: Error): number {
    if (!('mDetails' in e)) return -1;
    if (!(e.mDetails instanceof Object)) return -1;
    if (!('errorCode' in e.mDetails)) return -1;
    const r = e.mDetails.errorCode;
    if (typeof r !== 'number') return -1;
    return r;
}

const enum ErrorType {
    Unknown,
    ProfileProblem,
    NotFound,
    Cancel,
}

const ProfileErrorMessages = Object.freeze({
    'Expect Error: Required session must be defined': true,
    'Expect Error: Required object must be defined': true,
})

function analyzeError(e: any): ErrorType {
    if (!(e instanceof Error)) return ErrorType.Unknown;
    if (isCancellationError(e)) return ErrorType.Cancel;
    if (e.message in ProfileErrorMessages) // no comment
        return ErrorType.ProfileProblem;
    switch (getZoweErrorCode(e)) {
        case 401:
            return ErrorType.ProfileProblem;
        case 404:
            return ErrorType.NotFound;
        default:
            return ErrorType.Unknown;
    }
}


async function ZoweAsMFClient(info: ZoweConnectionInfo): Promise<MFClient> {
    let valid = true;

    function translateZoweError(e: any) {
        switch (analyzeError(e)) {
            case ErrorType.ProfileProblem:
                valid = false;
                throw new SuspendError(e);
            case ErrorType.NotFound:
                return null;
            default:
                throw e;
        }
    }

    const mvs = await ensureValidMfZoweClient<any>(info, info.zoweExplorerApi.getMvsApi).catch(e => { throw new SuspendError(e); });

    return {
        list: async (dataset: string): Promise<string[] | null> => {
            try {
                const { apiResponse: resp } = await mvs.allMembers(dataset);
                return resp.items.map((x: { member: string }) => x.member);
            } catch (e) {
                return translateZoweError(e);
            }
        },
        read: async (dataset: string, member: string): Promise<string | null> => {
            try {
                class StringWritable extends Writable {
                    private readonly chunks: Uint8Array[] = [];
                    _write(chunk: Buffer, encoding: BufferEncoding, callback: (error?: Error | null) => void) {
                        this.chunks.push(chunk);
                        callback();
                    }
                    getResult() { return textDecode(concat(...this.chunks)); }
                }
                const stream = new StringWritable();
                const content = await mvs.getContents(`${dataset}(${member})`, { stream });
                if (content.success)
                    return stream.getResult();
                else
                    return null;
            } catch (e) {
                return translateZoweError(e);
            }

        },
        close: () => { valid = false; },
        get closed() { return !valid; },
    }
}

function ftpProfileToServerId(info: FtpConnectionInfo): string {
    return info.host + ':' + (info.port ?? '21');
}

const defaultPorts = Object.freeze({
    http: 80,
    https: 443,
});

function knownProtocol(p: string): p is keyof typeof defaultPorts {
    return p in defaultPorts;
}

function zoweProfileToServerId(info: ZoweConnectionInfo): string | undefined {
    if (!('profile' in info.loadedProfile)) return undefined;
    const profile = info.loadedProfile.profile;
    if (typeof profile !== 'object' || !profile) return undefined;
    const host = 'host' in profile ? profile.host : undefined;
    const port = 'port' in profile ? profile.port : undefined;
    const protocol = 'protocol' in profile ? profile.protocol : undefined;
    if (!host || typeof host !== 'string') return undefined;
    if (typeof port === 'number')
        return host + ':' + port;
    if (port !== undefined)
        return undefined;
    if (typeof protocol !== 'string')
        return undefined;
    if (knownProtocol(protocol))
        return host + ':' + defaultPorts[protocol];

    return undefined;
}

async function FTPAsMFClient(info: FtpConnectionInfo): Promise<MFClient> {
    const client = new ftp.Client();
    client.parseList = (rawList: string): ftp.FileInfo[] => {
        return rawList.split(/\r?\n/).slice(1).filter(x => !/^\s*$/.test(x)).map(value => new ftp.FileInfo(value.split(/\s/, 1)[0]));
    };
    await client.access(translateConnectionInfo(info)).catch(e => { throw e instanceof ftp.FTPError && (e.code === 430 || e.code == 530) ? new SuspendError(e) : e; });

    return {
        list: async (dataset: string): Promise<string[] | null> => {
            try {
                await checkedCommand(client, 'TYPE A');
                const members = await client.list(`'${dataset}(*)'`);
                return members.map(x => x.name);
            }
            catch (e) {
                if (e instanceof ftp.FTPError && e.code == 550)
                    return null;
                throw e;
            }
        },
        read: async (dataset: string, member: string): Promise<string | null> => {
            try {
                const buffer = new FBWritable();
                buffer.on('error', err => { throw err });

                await checkedCommand(client, 'TYPE I');
                checkResponse(await client.downloadTo(buffer, `'${dataset}(${member})'`));

                return buffer.getResult();
            }
            catch (e) {
                if (e instanceof ftp.FTPError && e.code == 550)
                    return null;
                throw e;
            }
        },
        close: () => {
            client.close();
        },
        get closed() { return client.closed; },
    }
}

export function HLASMExternalFilesMF(context: vscode.ExtensionContext): ClientInterface<undefined, DatasetUriDetails, DatasetUriDetails> {
    let activeConnectionInfo: FtpConnectionInfo | ZoweConnectionInfo | undefined = undefined;

    const mutex = new AsyncMutex();
    const getConnInfo = async () => {
        const last = getLastRunConfig(context);
        const info = await gatherConnectionInfo(last);
        await updateLastRunConfig(context, { host: 'zoweExplorerApi' in info ? info.hostInput : info.host, user: info.user, jobcard: last.jobcard });

        return info;
    };
    let allowCredQuery = 0;
    let mirrorCredQuery = allowCredQuery;

    const pool = new ConnectionPool<MFClient>({
        create: async () => {
            return mutex.locked(async () => {
                if (mirrorCredQuery !== allowCredQuery)
                    throw new vscode.CancellationError();

                const info = activeConnectionInfo ?? (pool.closeClients(), await getConnInfo());
                activeConnectionInfo = undefined;
                const client: MFClient = 'zoweExplorerApi' in info ? await ZoweAsMFClient(info) : await FTPAsMFClient(info);
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
                const list = await client.list(args.dataset);
                if (!list) return null;
                return list.map(x => `/${args.dataset}/${x}.hlasm`);
            } catch (e) {
                if (e instanceof SuspendError)
                    activeConnectionInfo = undefined;
                throw e;
            }
        }),

        readMember: async (args: DatasetUriDetails): Promise<string | null> => pool.withClient(async (client) => {
            return client.read(args.dataset, args.member!).catch(e => {
                if (e instanceof SuspendError)
                    activeConnectionInfo = undefined;
                throw e;
            });
        }),

        serverId: () => mutex.locked(async () => {
            const transform = (arg: FtpConnectionInfo | ZoweConnectionInfo) => 'zoweExplorerApi' in arg ? zoweProfileToServerId(arg) : ftpProfileToServerId(arg);

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
