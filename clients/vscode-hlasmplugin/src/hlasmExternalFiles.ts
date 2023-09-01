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
import { uriFriendlyBase16Encode } from "./conversions";

import * as crypto from "crypto";
import { TextDecoder, TextEncoder, promisify } from "util";
import { deflate, inflate } from "zlib";

export const enum ExternalRequestType {
    read_file = 'read_file',
    list_directory = 'list_directory',
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

interface ExternalListDirectoryResponse {
    id: number,
    data: {
        member_urls: string[],
    },
}

type ExternalRequestDetails<R, L> = {
    [ExternalRequestType.read_file]: R,
    [ExternalRequestType.list_directory]: L,
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
    serverId?: () => string | undefined;
    toDisplayString?(): string;
}

export interface ExternalFilesInvalidationdata {
    paths: string[];
    serverId?: string;
}

export interface ClientInterface<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails> {
    parseArgs: (path: string, purpose: ExternalRequestType, query?: string) => Promise<{ details: ExternalRequestDetails<ReadArgs, ListArgs>[typeof purpose], server: ConnectArgs } | null>,

    listMembers(arg: ListArgs, server: ConnectArgs): Promise<string[] | null>;
    readMember(arg: ReadArgs, server: ConnectArgs): Promise<string | null>;

    invalidate?: vscode.Event<ExternalFilesInvalidationdata | undefined>;

    serverId?: () => Promise<string | undefined>;

    suspended?: () => void;
    resumed?: () => void;

    dispose?: () => void;
};

async function serverId<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(details: ClientUriDetails, instance: ClientInstance<ConnectArgs, ReadArgs, ListArgs>) {
    return details.serverId?.() ?? (instance.suspended ? undefined : await instance.client.serverId?.());
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

const not_exists = Object.freeze(new class { __stupid_typescript_workaround: undefined; });
const no_client = Object.freeze(new class { __stupid_typescript_workaround: undefined; });
interface InError { message: string };

const not_exists_json = new TextEncoder().encode(JSON.stringify("not_exists"));

const enum CachedResultType {
    string,
    arrayOfStrings,
};

function isValidCachedValue<R extends CachedResultType>(x: any, e: R): x is {
    [CachedResultType.string]: string,
    [CachedResultType.arrayOfStrings]: string[],
}[R] {
    if (e === CachedResultType.string) return typeof x === 'string';
    if (e === CachedResultType.arrayOfStrings) return Array.isArray(x) && x.every(y => typeof y === 'string');
    return false;
}

interface CacheEntry<T> {
    service: string | null,
    parsedArgs: ClientUriDetails | null,
    result: T | InError | typeof not_exists | typeof no_client,
    references: Set<string>;
    cached: boolean;
};

const cacheVersion = 'v2';

interface ClientInstance<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails> {
    suspended: boolean,
    client: Readonly<ClientInterface<ConnectArgs, ReadArgs, ListArgs>>,
    dispose: () => void,
};

function asFragment(s: string) { return s ? '#' + s : ''; }

export class HLASMExternalFiles {
    private toDispose: vscode.Disposable[] = [];

    private memberLists = new Map<string, CacheEntry<string[]>>();
    private memberContent = new Map<string, CacheEntry<string>>();

    private pendingRequests = new Set<{ topic: string }>();

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

    setClient<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(
        service: string,
        client: Readonly<ClientInterface<ConnectArgs, ReadArgs, ListArgs>>): vscode.Disposable {
        if (!/^[A-Z]+$/.test(service))
            throw Error('Invalid service name');

        if (this.clients.get(service))
            throw Error('Service already registered')

        const newClient: ClientInstance<ConnectArgs, ReadArgs, ListArgs> = {
            suspended: false,
            client: client,
            dispose: (() => {
                const toDispose = client.invalidate?.((list) => {
                    if (!list)
                        this.clearCache(service);
                    else
                        this.clearCache(service, list.paths, list.serverId);
                });
                return () => {
                    this.clients.delete(service);
                    toDispose?.dispose();
                    client.dispose?.();
                    this.notifyAllWorkspaces(service, true);
                }
            })(),
        };

        this.clients.set(service, newClient);

        this.notifyAllWorkspaces(service, false);

        return Object.freeze({
            dispose: () => newClient.dispose()
        });
    }

    private static readonly emptyUriDetails = Object.freeze({
        cacheKey: null,
        service: null,
        instance: null,
        details: null,
        server: null,
        associatedWorkspaceFragment: null,
    });

    private async extractUriDetails<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(uri: string | vscode.Uri, purpose: ExternalRequestType): Promise<{
        cacheKey: null;
        service: null;
        instance: null;
        details: null;
        server: null;
        associatedWorkspaceFragment: null;
    } | {
        cacheKey: string;
        service: string;
        instance: ClientInstance<ConnectArgs, ReadArgs, ListArgs>;
        details: ExternalRequestDetails<ReadArgs, ListArgs>[typeof purpose],
        server: ConnectArgs;
        associatedWorkspaceFragment: string;
    } | {
        cacheKey: string;
        service: string;
        instance: null;
        details: null;
        server: null;
        associatedWorkspaceFragment: string;
    }> {
        if (typeof uri === 'string')
            uri = vscode.Uri.parse(uri, true);

        const pathParser = /^\/([A-Z]+)(\/.*)?/;

        const matches = pathParser.exec(uri.path);

        if (uri.scheme !== this.magicScheme || !matches)
            return HLASMExternalFiles.emptyUriDetails;

        const service = matches[1];
        const subpath = matches[2] || '';

        const instance: ClientInstance<ConnectArgs, ReadArgs, ListArgs> | undefined = this.clients.get(service);
        const details_server = await instance?.client?.parseArgs(subpath, purpose, uri.query) ?? null;

        if (details_server && instance) {
            const { details, server } = details_server;
            return {
                cacheKey: `/${service}${details.normalizedPath()}`,
                service: service,
                instance: instance,
                details: details,
                server: server,
                associatedWorkspaceFragment: uri.fragment
            };
        }
        else
            return {
                cacheKey: encodeURI(uri.path),
                service: service,
                instance: null,
                details: null,
                server: null,
                associatedWorkspaceFragment: uri.fragment
            };
    }

    private prepareChangeNotification<T>(service: string, cache: Map<string, CacheEntry<T>>, all: boolean) {
        const changes = [...cache]
            .filter(([, v]) => (v.service === service) && (
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
                    uri: `${this.magicScheme}:/${service}${asFragment(uriFriendlyBase16Encode(w.uri.toString()))}`,
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
            client.client.suspended?.();
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
            client.client.resumed?.();
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
        [...this.clients.values()].forEach(x => x.dispose());
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

    private deriveCacheEntryName(serverId: string, service: string, normalizedPath: string) {
        return cacheVersion + '.' + service + '.' + crypto.createHash('sha256').update(JSON.stringify([
            serverId,
            normalizedPath
        ])).digest().toString('hex');
    }

    private async getCachedResult(serverId: string | undefined, service: string, normalizedPath: string, expect: CachedResultType.string): Promise<string | InError | typeof not_exists | undefined>
    private async getCachedResult(serverId: string | undefined, service: string, normalizedPath: string, expect: CachedResultType.arrayOfStrings): Promise<string[] | InError | typeof not_exists | undefined>
    private async getCachedResult(serverId: string | undefined, service: string, normalizedPath: string, expect: CachedResultType): Promise<unknown> {
        if (serverId === undefined || !this.cache) return undefined;

        const cacheEntryName = vscode.Uri.joinPath(this.cache.uri, this.deriveCacheEntryName(serverId, service, normalizedPath));

        try {
            const cachedResult = JSON.parse(new TextDecoder().decode(await promisify(inflate)(await this.cache.fs.readFile(cacheEntryName))));

            if (cachedResult === 'not_exists') return not_exists;
            if (cachedResult instanceof Object && 'data' in cachedResult) {
                const data = cachedResult.data;

                if (isValidCachedValue(data, expect))
                    return data;
            }
        }
        catch (e) { }

        return undefined;
    }

    private async storeCachedResult<T>(serverId: string | undefined, service: string, normalizedPath: string, value: T | typeof not_exists) {
        if (serverId === undefined || !this.cache) return false;

        const cacheEntryName = vscode.Uri.joinPath(this.cache.uri, this.deriveCacheEntryName(serverId, service, normalizedPath));

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

    private async getFile<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(instance: ClientInstance<ConnectArgs, ReadArgs, ListArgs>, service: string, [path, connArgs]: [ReadArgs, ConnectArgs]): Promise<string | InError | typeof not_exists | typeof no_client | null> {
        try {
            const normalizedPath = path.normalizedPath();
            return await this.getCachedResult(await serverId(path, instance), service, normalizedPath, CachedResultType.string)
                ?? (instance.suspended
                    ? no_client
                    : await this.askClient(service, path.toDisplayString?.() ?? normalizedPath,
                        () => instance.client.readMember(path, connArgs)
                    )
                );
        }
        catch (x) { return this.handleError(x); }
    }

    private async getDir<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(instance: ClientInstance<ConnectArgs, ReadArgs, ListArgs>, service: string, [path, connArgs]: [ListArgs, ConnectArgs]): Promise<string[] | InError | typeof not_exists | typeof no_client | null> {
        try {
            const normalizedPath = path.normalizedPath();
            return await this.getCachedResult(await serverId(path, instance), service, normalizedPath, CachedResultType.arrayOfStrings)
                ?? (instance.suspended
                    ? no_client
                    : await this.askClient(service, path.toDisplayString?.() ?? normalizedPath,
                        () => instance.client.listMembers(path, connArgs)
                    )
                );
        }
        catch (x) { return this.handleError(x); }
    }

    private async handleMessage<T, ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(
        msg: ExternalRequest,
        getData: (client: ClientInstance<ConnectArgs, ReadArgs, ListArgs>, service: string, details: [ExternalRequestDetails<ReadArgs, ListArgs>[typeof msg.op], ConnectArgs]) => Promise<CacheEntry<T>['result'] | null>,
        inMemoryCache: Map<string, CacheEntry<T>>,
        responseTransform: (result: T, pathTransform: (p: string) => string) => (T extends string[] ? ExternalListDirectoryResponse : ExternalReadFileResponse)['data']):
        Promise<(T extends string[] ? ExternalListDirectoryResponse : ExternalReadFileResponse) | ExternalErrorResponse | null> {
        if (msg.op !== ExternalRequestType.read_file && msg.op !== ExternalRequestType.list_directory) throw Error("");
        const { cacheKey, service, instance, details, server, associatedWorkspaceFragment } = await this.extractUriDetails<ConnectArgs, ReadArgs, ListArgs>(msg.url, msg.op);
        if (!cacheKey || instance && !details)
            return this.generateError(msg.id, -5, 'Invalid request');

        let content = inMemoryCache.get(cacheKey);
        if (content === undefined) {
            const result = instance ? await getData(instance, service, [details, server]) : no_client;
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

        const { response, cache } = await this.transformResult(msg.id, content, x => responseTransform(x, x => `${this.magicScheme}:/${service}${x}${asFragment(associatedWorkspaceFragment)}`));

        if (cache && instance && !content.cached)
            content.cached = await this.storeCachedResult(await serverId(details, instance), service, details.normalizedPath(), content.result);

        return response;
    }

    private generateError(id: number, code: number, msg: string): ExternalErrorResponse {
        return { id, error: { code, msg } };
    }

    private transformResult<T>(
        id: number,
        content: CacheEntry<T>,
        transform: (result: T) => (T extends string[] ? ExternalListDirectoryResponse : ExternalReadFileResponse)['data']
    ): Promise<{
        response: (T extends string[] ? ExternalListDirectoryResponse : ExternalReadFileResponse) | ExternalErrorResponse,
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
                response: <T extends string[] ? ExternalListDirectoryResponse : ExternalReadFileResponse>{
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
        return this.handleMessage(msg, this.getDir.bind(this), this.memberLists, (result, urlTransform) => {
            return {
                member_urls: result.map(x => urlTransform(x)),
            };
        });
    }

    public handleRawMessage(msg: any): Promise<ExternalReadFileResponse | ExternalListDirectoryResponse | ExternalErrorResponse | null> {
        if (!msg || typeof msg.id !== 'number' || typeof msg.op !== 'string')
            return Promise.resolve(null);

        if (msg.op === ExternalRequestType.read_file && typeof msg.url === 'string')
            return this.handleFileMessage(msg);
        if (msg.op === ExternalRequestType.list_directory && typeof msg.url === 'string')
            return this.handleDirMessage(msg);

        return Promise.resolve(this.generateError(msg.id, -5, 'Invalid request'));
    }

    public async clearCache(service?: string, paths?: string[], serverId?: string) {
        if (this.cache) {
            const prefix = service && cacheVersion + '.' + service + '.';
            const useServerId = serverId ?? (service && await this.clients.get(service)?.client.serverId?.());
            const cacheKeys = paths && service && useServerId && new Set(paths.map(x => this.deriveCacheEntryName(useServerId, service, x)));
            const { uri, fs } = this.cache;

            const files = await fs.readDirectory(uri);

            let errors = 0;
            const pending = new Set<Promise<void>>();

            for (const [filename] of files) {
                if (prefix && !filename.startsWith(prefix))
                    continue;
                if (cacheKeys && !cacheKeys.has(filename))
                    continue;

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
        if (service)
            this.notifyAllWorkspaces(service, true);
        else
            this.clients.forEach((_, key) => this.notifyAllWorkspaces(key, true));
    }

    public currentlyAvailableServices() {
        return [...this.clients.keys()];
    }
}
