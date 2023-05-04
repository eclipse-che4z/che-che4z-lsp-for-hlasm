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

import { Disposable } from "vscode";
import { AsyncSemaphore } from "./asyncMutex";

export class ConnectionPool<T> implements Disposable {
    private pooledClients: T[] = [];
    private pooledClientsTimeout: ReturnType<typeof setTimeout> | null = null;

    private semaphore: AsyncSemaphore;
    private disposed = false;
    private generation = 0;

    constructor(private clientDetails: {
        create: () => T | PromiseLike<T>,
        reusable: (client: T) => boolean,
        close: (client: T) => void
    }, limit: number, private timeout: number) {
        this.semaphore = new AsyncSemaphore(limit);
    }

    dispose() {
        this.disposed = true;
        this.clearTimeout();
        this.closeClients();
    }

    public closeClients() {
        ++this.generation;
        this.pooledClients.forEach(x => this.clientDetails.close(x));
        this.pooledClients = [];
    }

    clearTimeout() {
        if (this.pooledClientsTimeout) {
            clearTimeout(this.pooledClientsTimeout);
            this.pooledClientsTimeout = null;
        }
    }

    resetTimeout() {
        this.clearTimeout();
        this.pooledClientsTimeout = setTimeout(() => {
            this.pooledClientsTimeout = null;
            this.closeClients();
        }, this.timeout);
    }

    private async getClient() {
        this.clearTimeout();

        if (this.pooledClients.length === 0)
            return Promise.resolve(this.clientDetails.create());

        return this.pooledClients.shift()!;
    }

    async withClient<R>(action: (c: T) => R | PromiseLike<R>): Promise<R> {
        return this.semaphore.locked(async () => {
            const client = await this.getClient();
            const gen = this.generation;
            try {
                const result = await Promise.resolve(action(client));

                if (gen === this.generation && !this.disposed && this.clientDetails.reusable(client)) {
                    this.pooledClients.push(client);
                    this.resetTimeout();
                }
                else
                    this.clientDetails.close(client);

                return result;
            }
            catch (e) {
                this.clientDetails.close(client);
                throw e;
            }
        });
    }
}
