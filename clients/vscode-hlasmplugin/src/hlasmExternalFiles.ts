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

import * as vscode from "vscode";
import * as vscodelc from "vscode-languageclient";
import { isCancellationError } from "./helpers";
import { uriFriendlyBase16Decode, uriFriendlyBase16Encode } from "./conversions";

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
interface ExternalErrorResponse {
    id: number,
    error: {
        code: number,
        msg: string,
    },
}

export interface ClientUriDetails {
    toString(): string;
    normalizedPath(): string;
}

export interface ExternalFilesClient extends vscode.Disposable {
    listMembers(arg: ClientUriDetails): Promise<string[] | null>;
    readMember(arg: ClientUriDetails): Promise<string | null>;

    onStateChange: vscode.Event<boolean>;

    suspend(): void;
    resume(): void;

    suspended(): boolean;

    parseArgs(path: string, purpose: ExternalRequestType): ClientUriDetails | null;
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

interface CacheEntry<T> {
    service: string | null,
    parsedArgs: ClientUriDetails | null,
    result: T | InError | typeof not_exists | typeof no_client,
    references: Set<string>;
};
export class HLASMExternalFiles {
    private toDispose: vscode.Disposable[] = [];

    private memberLists = new Map<string, CacheEntry<string[]>>();
    private memberContent = new Map<string, CacheEntry<string>>();

    private pendingRequests = new Set<{ topic: string }>();

    private clients = new Map<string, {
        client: ExternalFilesClient;
        clientDisposables: vscode.Disposable[];
    }>();

    constructor(private magicScheme: string, private channel: {
        onNotification(method: string, handler: vscodelc.GenericNotificationHandler): vscode.Disposable;
        sendNotification<P>(type: vscodelc.NotificationType<P>, params?: P): Promise<void>;
        sendNotification(method: string, params: any): Promise<void>;
    }) {
        this.toDispose.push(this.channel.onNotification('external_file_request', params => this.handleRawMessage(params).then(
            msg => { if (msg) this.channel.sendNotification('external_file_response', msg); }
        )));
    }

    setClient(service: string, client: ExternalFilesClient | null) {
        if (!/^[A-Z]+$/.test(service))
            throw Error('Invalid service name');

        const oldClient = this.clients.get(service);
        if (oldClient) {
            this.clients.delete(service);

            oldClient.clientDisposables.forEach(x => x.dispose());
            oldClient.clientDisposables = [];
            oldClient.client.dispose();
        }

        if (!client) return;

        const newClient = {
            client: client,
            clientDisposables: [
                client.onStateChange((suspended) => {
                    if (suspended) {
                        if (this.activeProgress) {
                            clearTimeout(this.pendingActiveProgressCancellation);
                            this.activeProgress.done();
                            this.activeProgress = null;
                        }
                        vscode.window.showInformationMessage("Retrieval of remote files has been suspended.");
                    }
                    else
                        this.notifyAllWorkspaces(service, false);
                })
            ]
        };
        this.clients.set(service, newClient);

        if (oldClient || !client.suspended())
            this.notifyAllWorkspaces(service, !!oldClient);
    }

    private getClient(service: string) {
        return this.clients.get(service)?.client;
    }

    private static readonly emptyUriDetails = Object.freeze({
        cacheKey: null,
        service: null,
        client: null,
        details: null,
        associatedWorkspace: null,
    });
    private extractUriDetails(uri: string | vscode.Uri, purpose: ExternalRequestType): {
        cacheKey: null;
        service: null;
        client: null;
        details: null;
        associatedWorkspace: null;
    } | {
        cacheKey: string;
        service: string;
        client: ExternalFilesClient;
        details: ClientUriDetails;
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

        const client = this.clients.get(service);
        const details = client?.client.parseArgs(subpath, purpose) ?? null;

        if (details) {
            return {
                cacheKey: `/${service}${details.normalizedPath()}`,
                service: service,
                client: client!.client,
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
        if (!this.activeProgress && !this.getClient(service)!.suspended()) {
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
        this.clients.forEach(({ client }) => { client.suspend() });
    }

    public resumeAll() {
        this.clients.forEach(({ client }) => { client.resume() });
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
        parsedArgs: ClientUriDetails,
        func: (args: ClientUriDetails) => Promise<T | null>
    ): Promise<T | InError | typeof not_exists | typeof no_client | null> {
        const interest = this.addWIP(service, parsedArgs.toString());

        try {
            const result = await func(parsedArgs);

            if (!interest()) return null;

            if (!result)
                return not_exists;

            return result;

        } catch (x) {
            const e = x instanceof Error ? x : Error('Unknown error:' + x);
            if (!isCancellationError(e)) {
                this.suspendAll();
                vscode.window.showErrorMessage(e.message);
            }

            if (!interest()) return null;

            return { message: e.message };
        }

    }

    private async getFile(client: ExternalFilesClient, service: string, parsedArgs: ClientUriDetails): Promise<string | InError | typeof not_exists | typeof no_client | null> {
        return this.askClient(service, parsedArgs, client.readMember.bind(client));
    }

    private async getDir(client: ExternalFilesClient, service: string, parsedArgs: ClientUriDetails): Promise<string[] | InError | typeof not_exists | typeof no_client | null> {
        return this.askClient(service, parsedArgs, client.listMembers.bind(client));
    }

    private async handleMessage<T>(
        msg: ExternalRequest,
        getData: (client: ExternalFilesClient, service: string, details: ClientUriDetails) => Promise<CacheEntry<T>['result'] | null>,
        cache: Map<string, CacheEntry<T>>,
        responseTransform: (result: T) => (T extends string[] ? ExternalReadDirectoryResponse : ExternalReadFileResponse)['data']):
        Promise<(T extends string[] ? ExternalReadDirectoryResponse : ExternalReadFileResponse) | ExternalErrorResponse | null> {
        const { cacheKey, service, client, details } = this.extractUriDetails(msg.url, msg.op);
        if (!cacheKey || client && !details)
            return this.generateError(msg.id, -5, 'Invalid request');

        let content = cache.get(cacheKey);
        if (content === undefined) {
            const result = client ? await getData(client, service, details) : no_client;
            if (!result) return Promise.resolve(null);
            content = {
                service: service,
                parsedArgs: details,
                result: result,
                references: new Set<string>(),
            };

            cache.set(cacheKey, content);
        }
        content.references.add(msg.url);

        return this.transformResult(msg.id, content, responseTransform);
    }

    private generateError(id: number, code: number, msg: string): ExternalErrorResponse {
        return { id, error: { code, msg } };
    }

    private transformResult<T>(
        id: number,
        content: CacheEntry<T>,
        transform: (result: T) => (T extends string[] ? ExternalReadDirectoryResponse : ExternalReadFileResponse)['data']
    ): Promise<(T extends string[] ? ExternalReadDirectoryResponse : ExternalReadFileResponse) | ExternalErrorResponse> {
        if (content.result === not_exists)
            return Promise.resolve(this.generateError(id, 0, 'Not found'));
        else if (content.result === no_client)
            return Promise.resolve(this.generateError(id, -1000, 'No client'));
        else if (content.result instanceof Object && 'message' in content.result)
            return Promise.resolve(this.generateError(id, -1000, content.result.message));
        else
            return Promise.resolve(<T extends string[] ? ExternalReadDirectoryResponse : ExternalReadFileResponse>{
                id,
                data: transform(<T>content.result),
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

}
