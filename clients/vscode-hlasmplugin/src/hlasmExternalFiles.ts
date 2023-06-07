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
import { asError, isCancellationError } from "./helpers";
import { uriFriendlyBase16Decode, uriFriendlyBase16Encode } from "./conversions";

import * as crypto from "crypto";
import { TextDecoder, TextEncoder, promisify } from "util";
import { deflate, inflate } from "zlib";
import { ConnectionPool } from "./connectionPool";
import { AsyncMutex } from "./asyncMutex";

// This is a temporary demo implementation

export enum ExternalRequestType {
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
    data: {
        members: string[],
        suggested_extension: string | undefined,
    },
}

type ExternalRequestDetails<R, L> = {
    [ExternalRequestType.read_file]: R,
    [ExternalRequestType.read_directory]: L,
};

interface ExternalErrorResponse {
    id: number,
    error: {
        code: number,
        msg: string,
    },
}

export interface ClientUriDetails {
    normalizedPath(): string;
    toDisplayString?(): string;
}

export interface ClientInterface<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails> {
    getConnInfo: () => Promise<{ info: ConnectArgs, uniqueId?: string }>,
    parseArgs: (path: string, purpose: ExternalRequestType) => ExternalRequestDetails<ReadArgs, ListArgs>[typeof purpose] | null,
    createClient: () => ExternalFilesClient<ConnectArgs, ReadArgs, ListArgs>
};

export interface ExternalFilesClient<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails> {
    dispose(): void;

    connect(arg: ConnectArgs): Promise<void>;

    listMembers(arg: ListArgs): Promise<string[] | null>;
    readMember(arg: ReadArgs): Promise<string | null>;

    reusable: () => boolean;
}

function take<T>(it: IterableIterator<T>, n: number): T[] {
    const result: T[] = [];
    while (n) {
        const val = it.next();
        if (val.done)
            break;
        result.push(val.value);
        --n;
    }
    return result;
}

const not_exists = Object.freeze({});
const no_client = Object.freeze({});
interface InError { message: string };

const not_exists_json = new TextEncoder().encode(JSON.stringify("not_exists"));

interface CacheEntry<T> {
    service: string | null,
    parsedArgs: ClientUriDetails | null,
    result: T | InError | typeof not_exists | typeof no_client,
    references: Set<string>;
    cached: boolean;
};

const cacheVersion = 'v1';

const connectionPoolSize = 4;
const connectionPoolTimeout = 30000;

interface ClientInstance<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails> {
    activeConnectionInfo: { info: ConnectArgs, uniqueId?: string } | null;
    pool: ConnectionPool<ExternalFilesClient<ConnectArgs, ReadArgs, ListArgs>>,
    suspended: boolean,
    parseArgs: (path: string, purpose: ExternalRequestType) => ExternalRequestDetails<ReadArgs, ListArgs>[typeof purpose] | null,
    ensureConnectionInfo: () => Promise<string | undefined>,
};

export class HLASMExternalFiles {
    private toDispose: vscode.Disposable[] = [];

    private memberLists = new Map<string, CacheEntry<string[]>>();
    private memberContent = new Map<string, CacheEntry<string>>();

    private pendingRequests = new Set<{ topic: string }>();

    private mutex = new AsyncMutex();
    private clients = new Map<string, ClientInstance<any, any, any>>();

    constructor(
        private magicScheme: string,
        private channel: {
            onNotification(method: string, handler: vscodelc.GenericNotificationHandler): vscode.Disposable;
            sendNotification<P>(type: vscodelc.NotificationType<P>, params?: P): Promise<void>;
            sendNotification(method: string, params: any): Promise<void>;
        },
        private cache?: { uri: vscode.Uri, fs: vscode.FileSystem }) {
        this.toDispose.push(this.channel.onNotification('external_file_request', params => this.handleRawMessage(params).then(
            msg => { if (msg) this.channel.sendNotification('external_file_response', msg); }
        )));
    }

    private async getConnectedClient<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(
        createClient: () => ExternalFilesClient<ConnectArgs, ReadArgs, ListArgs>,
        getConnInfo: () => Promise<{ info: ConnectArgs, uniqueId?: string }>,
        clientType: ClientInstance<ConnectArgs, ReadArgs, ListArgs>) {
        const client = createClient();
        return await this.mutex.locked(async () => {
            while (true) {
                try {
                    if (clientType.suspended)
                        throw new vscode.CancellationError();
                    const connection = clientType.activeConnectionInfo ?? await getConnInfo();
                    clientType.activeConnectionInfo = null;
                    try {
                        await client.connect(connection.info);
                    }
                    catch (e) {
                        vscode.window.showErrorMessage(asError(e).message);
                        continue;
                    }
                    clientType.activeConnectionInfo = connection;
                    return client;
                }
                catch (e) {
                    this.suspendAll(); // needs to be done with lock held to prevent showing UI elements
                    try { client.dispose(); } catch (_) { }
                    throw e;
                }
            }
        });
    }

    setClient<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(
        service: string,
        client: Readonly<ClientInterface<ConnectArgs, ReadArgs, ListArgs>> | null) {
        if (!/^[A-Z]+$/.test(service))
            throw Error('Invalid service name');

        const oldClient = this.clients.get(service);
        if (oldClient) {
            this.clients.delete(service);
            oldClient.pool.dispose();
        }

        if (!client) return;

        const newClient: ClientInstance<ConnectArgs, ReadArgs, ListArgs> = {
            activeConnectionInfo: null,
            suspended: false,
            pool: new ConnectionPool({
                create: () => this.getConnectedClient(client.createClient, client.getConnInfo, newClient),
                reusable: (c) => !newClient.suspended && c.reusable(),
                close: (c) => { try { c.dispose(); } catch (e) { } },
            }, connectionPoolSize, connectionPoolTimeout),
            parseArgs: client.parseArgs,
            ensureConnectionInfo: () => this.mutex.locked(async () => {
                if (newClient.suspended) return undefined;
                if (newClient.activeConnectionInfo) return newClient.activeConnectionInfo.uniqueId;
                try {
                    newClient.activeConnectionInfo = await client.getConnInfo();
                    return newClient.activeConnectionInfo.uniqueId;
                }
                catch (e) {
                    this.suspendAll(); // needs to be done with lock held to prevent showing UI elements
                    throw e;
                }
            }),
        };
        this.clients.set(service, newClient);

        this.notifyAllWorkspaces(service, !!oldClient);
    }

    private static readonly emptyUriDetails = Object.freeze({
        cacheKey: null,
        service: null,
        client: null,
        details: null,
        associatedWorkspace: null,
    });

    private extractUriDetails<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(uri: string | vscode.Uri, purpose: ExternalRequestType): {
        cacheKey: null;
        service: null;
        client: null;
        details: null;
        associatedWorkspace: null;
    } | {
        cacheKey: string;
        service: string;
        client: ClientInstance<ConnectArgs, ReadArgs, ListArgs>;
        details: ExternalRequestDetails<ReadArgs, ListArgs>[typeof purpose],
        associatedWorkspace: string;
    } | {
        cacheKey: string;
        service: null;
        client: null;
        details: null;
        associatedWorkspace: string;
    } {
        if (typeof uri === 'string')
            uri = vscode.Uri.parse(uri, true);

        const pathParser = /^\/([A-Z]+)(\/.*)?/;

        const matches = pathParser.exec(uri.path);

        if (uri.scheme !== this.magicScheme || !matches)
            return HLASMExternalFiles.emptyUriDetails;

        const service = matches[1];
        const subpath = matches[2] || '';

        const client: ClientInstance<ConnectArgs, ReadArgs, ListArgs> | undefined = this.clients.get(service);
        const details = client?.parseArgs(subpath, purpose) ?? null;

        if (details && client) {
            return {
                cacheKey: `/${service}${details.normalizedPath()}`,
                service: service,
                client: client,
                details: details,
                associatedWorkspace: uriFriendlyBase16Decode(uri.authority)
            };
        }
        else
            return {
                cacheKey: uri.path,
                service: null,
                client: null,
                details: null,
                associatedWorkspace: uriFriendlyBase16Decode(uri.authority)
            };
    }

    private prepareChangeNotification<T>(service: string, cache: Map<string, CacheEntry<T>>, all: boolean) {
        const changes = [...cache]
            .filter(([, v]) => v.service === service && (
                all ||
                v.result instanceof Object && (v.result === no_client || 'message' in v.result)
            ))
            .map(([path, value]) => { return { path: path, refs: value.references }; });

        changes.forEach(x => cache.delete(x.path));

        return changes.map(x => [...x.refs]).flat().map(x => {
            return {
                uri: x,
                type: vscodelc.FileChangeType.Changed
            };
        });
    }

    private notifyAllWorkspaces(service: string, all: boolean) {
        this.channel.sendNotification(vscodelc.DidChangeWatchedFilesNotification.type, {
            changes: (vscode.workspace.workspaceFolders || []).map(w => {
                return {
                    uri: `${this.magicScheme}://${uriFriendlyBase16Encode(w.uri.toString())}/${service}`,
                    type: vscodelc.FileChangeType.Changed
                };
            })
                .concat(this.prepareChangeNotification(service, this.memberContent, all))
                .concat(this.prepareChangeNotification(service, this.memberLists, all))
        });
    }

    activeProgress: { progressUpdater: vscode.Progress<{ message?: string; increment?: number }>, done: () => void } | null = null;
    pendingActiveProgressCancellation: ReturnType<typeof setTimeout> = setTimeout(() => { }, 0);
    addWIP(service: string, topic: string) {
        clearTimeout(this.pendingActiveProgressCancellation);
        if (!this.activeProgress && !this.clients.get(service)!.suspended) {
            vscode.window.withProgress({ title: 'Retrieving remote files', location: vscode.ProgressLocation.Notification }, (progress, c) => {
                return new Promise<void>((resolve) => {
                    this.activeProgress = { progressUpdater: progress, done: resolve };
                });
            });
        }
        const wip = { topic };
        this.pendingRequests.add(wip);

        if (this.activeProgress)
            this.activeProgress.progressUpdater.report({
                message: take(this.pendingRequests.values(), 3)
                    .map((v, n) => { return n < 2 ? v.topic : '...' })
                    .join(', ')
            });

        return () => {
            const result = this.pendingRequests.delete(wip);

            if (this.activeProgress && this.pendingRequests.size === 0) {
                this.pendingActiveProgressCancellation = setTimeout(() => {
                    this.activeProgress!.done();
                    this.activeProgress = null;
                }, 2500);
            }

            return result;
        };
    }

    public suspendAll() {
        let oneSuspended = false;
        this.clients.forEach(client => {
            if (client.suspended) return;
            client.suspended = oneSuspended = true;
            client.pool.closeClients();
        });

        if (oneSuspended) {
            if (this.activeProgress) {
                clearTimeout(this.pendingActiveProgressCancellation);
                this.activeProgress.done();
                this.activeProgress = null;
            }
            vscode.window.showInformationMessage("Retrieval of remote files has been suspended.");
        }
    }

    public resumeAll() {
        this.clients.forEach((client, service) => {
            if (!client.suspended) return;
            client.suspended = false;
            this.notifyAllWorkspaces(service, false);
        });
    }

    public getTextDocumentContentProvider(): vscode.TextDocumentContentProvider {
        const me = this;
        return {
            async provideTextDocumentContent(uri: vscode.Uri, token: vscode.CancellationToken): Promise<string | null> {
                const result = await me.handleFileMessage({ url: uri.toString(), id: -1, op: ExternalRequestType.read_file });
                if (result && 'data' in result && typeof result.data === 'string')
                    return result.data;
                else
                    return null;
            }
        }
    }

    public reset() { this.pendingRequests.clear(); }

    dispose() {
        this.toDispose.forEach(x => x.dispose());
        [...this.clients.keys()].forEach(x => this.setClient(x, null));
    }

    private async askClient<T>(
        service: string,
        argsAsString: string,
        func: () => Promise<T | null>
    ): Promise<T | InError | typeof not_exists | typeof no_client | null> {
        const interest = this.addWIP(service, argsAsString);

        try {
            const result = await func();

            if (!interest()) return null;

            if (!result)
                return not_exists;

            return result;

        } catch (e) {
            if (!interest()) return null;
            throw e;
        }
    }

    private deriveCacheEntryName(clientId: string, service: string, normalizedPath: string) {
        return cacheVersion + '.' + crypto.createHash('sha256').update(JSON.stringify([
            clientId,
            service,
            normalizedPath
        ])).digest().toString('hex');
    }

    private async getCachedResult<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(client: ClientInstance<ConnectArgs, ReadArgs, ListArgs>, service: string, normalizedPath: string, expect: 'string'): Promise<string | InError | typeof not_exists | undefined>;
    private async getCachedResult<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(client: ClientInstance<ConnectArgs, ReadArgs, ListArgs>, service: string, normalizedPath: string, expect: 'arrayOfStrings'): Promise<string[] | InError | typeof not_exists | undefined>;
    private async getCachedResult<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(client: ClientInstance<ConnectArgs, ReadArgs, ListArgs>, service: string, normalizedPath: string, expect: 'string' | 'arrayOfStrings') {
        if (!this.cache) return undefined;
        const clientId = await client.ensureConnectionInfo();
        if (!clientId) return undefined;

        const cacheEntryName = vscode.Uri.joinPath(this.cache.uri, this.deriveCacheEntryName(clientId, service, normalizedPath));

        try {
            const cachedResult = JSON.parse(new TextDecoder().decode(await promisify(inflate)(await this.cache.fs.readFile(cacheEntryName))));

            if (cachedResult === 'not_exists') return not_exists;
            if (cachedResult instanceof Object && 'data' in cachedResult) {
                const data = cachedResult.data;

                if (expect === 'string' && typeof data === 'string')
                    return data;
                if (expect === 'arrayOfStrings' && Array.isArray(data) && data.every(x => typeof x === 'string'))
                    return data;
            }
        }
        catch (e) { }

        return undefined;
    }

    private async storeCachedResult<T, ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(client: ClientInstance<ConnectArgs, ReadArgs, ListArgs>, service: string, normalizedPath: string, value: T | typeof not_exists) {
        if (!this.cache) return false;
        const clientId = client.activeConnectionInfo?.uniqueId;
        if (!clientId) return false;

        const cacheEntryName = vscode.Uri.joinPath(this.cache.uri, this.deriveCacheEntryName(clientId, service, normalizedPath));

        try {
            const data = value === not_exists
                ? not_exists_json
                : new TextEncoder().encode(JSON.stringify({ data: value }));

            await this.cache.fs.writeFile(cacheEntryName, await promisify(deflate)(data));
            return true;
        }
        catch (e) { }

        return false;
    }

    private handleError(x: unknown) {
        const e = asError(x);
        this.suspendAll();
        if (!isCancellationError(e))
            vscode.window.showErrorMessage(e.message);

        return { message: e.message };
    }

    private async getFile<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(client: ClientInstance<ConnectArgs, ReadArgs, ListArgs>, service: string, parsedArgs: ReadArgs): Promise<string | InError | typeof not_exists | typeof no_client | null> {
        try {
            const normalizedPath = parsedArgs.normalizedPath();
            return await this.getCachedResult(client, service, normalizedPath, 'string') ?? await this.askClient(service, parsedArgs.toDisplayString?.() ?? normalizedPath, () => client.pool.withClient(c => c.readMember(parsedArgs)));
        }
        catch (x) { return this.handleError(x); }
    }

    private async getDir<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(client: ClientInstance<ConnectArgs, ReadArgs, ListArgs>, service: string, parsedArgs: ListArgs): Promise<string[] | InError | typeof not_exists | typeof no_client | null> {
        try {
            const normalizedPath = parsedArgs.normalizedPath();
            return await this.getCachedResult(client, service, normalizedPath, 'arrayOfStrings') ?? await this.askClient(service, parsedArgs.toDisplayString?.() ?? normalizedPath, () => client.pool.withClient(c => c.listMembers(parsedArgs)));
        }
        catch (x) { return this.handleError(x); }
    }

    private async handleMessage<T, ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(
        msg: ExternalRequest,
        getData: (client: ClientInstance<ConnectArgs, ReadArgs, ListArgs>, service: string, details: ExternalRequestDetails<ReadArgs, ListArgs>[typeof msg.op]) => Promise<CacheEntry<T>['result'] | null>,
        inMemoryCache: Map<string, CacheEntry<T>>,
        responseTransform: (result: T) => (T extends string[] ? ExternalReadDirectoryResponse : ExternalReadFileResponse)['data']):
        Promise<(T extends string[] ? ExternalReadDirectoryResponse : ExternalReadFileResponse) | ExternalErrorResponse | null> {
        if (msg.op !== ExternalRequestType.read_file && msg.op !== ExternalRequestType.read_directory) throw Error("");
        const { cacheKey, service, client, details } = this.extractUriDetails<ConnectArgs, ReadArgs, ListArgs>(msg.url, msg.op);
        if (!cacheKey || client && !details)
            return this.generateError(msg.id, -5, 'Invalid request');

        let content = inMemoryCache.get(cacheKey);
        if (content === undefined) {
            const result = client ? await getData(client, service, details) : no_client;
            if (!result) return Promise.resolve(null);
            content = {
                service: service,
                parsedArgs: details,
                result: result,
                references: new Set<string>(),
                cached: false,
            };

            inMemoryCache.set(cacheKey, content);
        }
        content.references.add(msg.url);

        const { response, cache } = await this.transformResult(msg.id, content, responseTransform);

        if (cache && client && !content.cached)
            content.cached = await this.storeCachedResult(client, service, details.normalizedPath(), content.result);

        return response;
    }

    private generateError(id: number, code: number, msg: string): ExternalErrorResponse {
        return { id, error: { code, msg } };
    }

    private transformResult<T>(
        id: number,
        content: CacheEntry<T>,
        transform: (result: T) => (T extends string[] ? ExternalReadDirectoryResponse : ExternalReadFileResponse)['data']
    ): Promise<{
        response: (T extends string[] ? ExternalReadDirectoryResponse : ExternalReadFileResponse) | ExternalErrorResponse,
        cache: boolean
    }> {
        if (content.result === not_exists)
            return Promise.resolve({ response: this.generateError(id, 0, 'Not found'), cache: true });
        else if (content.result === no_client)
            return Promise.resolve({ response: this.generateError(id, -1000, 'No client'), cache: false });
        else if (content.result instanceof Object && 'message' in content.result)
            return Promise.resolve({ response: this.generateError(id, -1000, content.result.message), cache: false });
        else
            return Promise.resolve({
                response: <T extends string[] ? ExternalReadDirectoryResponse : ExternalReadFileResponse>{
                    id,
                    data: transform(<T>content.result),
                },
                cache: true
            });
    }

    private handleFileMessage(msg: ExternalRequest) {
        return this.handleMessage(msg, this.getFile.bind(this), this.memberContent, x => x);
    }
    private handleDirMessage(msg: ExternalRequest) {
        return this.handleMessage(msg, this.getDir.bind(this), this.memberLists, (result) => {
            return {
                members: result,
                suggested_extension: '.hlasm',
            };
        });
    }

    public handleRawMessage(msg: any): Promise<ExternalReadFileResponse | ExternalReadDirectoryResponse | ExternalErrorResponse | null> {
        if (!msg || typeof msg.id !== 'number' || typeof msg.op !== 'string')
            return Promise.resolve(null);

        if (msg.op === ExternalRequestType.read_file && typeof msg.url === 'string')
            return this.handleFileMessage(msg);
        if (msg.op === ExternalRequestType.read_directory && typeof msg.url === 'string')
            return this.handleDirMessage(msg);

        return Promise.resolve(this.generateError(msg.id, -5, 'Invalid request'));
    }

    public async clearCache() {
        if (this.cache) {
            const { uri, fs } = this.cache;

            const files = await fs.readDirectory(uri);

            let errors = 0;
            const pending = new Set<Promise<void>>();

            for (const [filename] of files) {
                const p = Promise.resolve(fs.delete(vscode.Uri.joinPath(uri, filename)));
                pending.add(p);
                p.catch(() => ++errors).finally(() => pending.delete(p));

                if (pending.size > 16)
                    await Promise.race(pending);
            }
            await Promise.allSettled(pending);

            if (errors > 0)
                vscode.window.showErrorMessage(`Errors (${errors}) occurred while clearing out the cache`);
        }
        for (const [service] of this.clients)
            this.notifyAllWorkspaces(service, true);
    }
}
