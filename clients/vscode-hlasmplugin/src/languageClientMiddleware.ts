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

import { Middleware } from 'vscode-languageclient';

export function getLanguageClientMiddleware(): Middleware {
    // VSCode generates flood of didOpen/didClose event when trying to go to a symbol definition using mouse (CTRL+point).
    // This behavior is documented (and apprently work on) but it causes meaningless activity in the language server.

    // The purpose of the middleware is to delay sending the didOpen event by a fraction of a second
    // and attempt to pair it with didClose generated almost immediatelly after.
    // In that case, we drop both events.

    const pendingOpens = new Map<string, {
        send: () => void,
        clearTimeout: () => void,
    }>();
    const timeout = 50;

    return {
        didOpen: (data, next) => {
            const uri = data.uri.toString();

            const didOpenEvent = pendingOpens.get(uri);
            if (didOpenEvent) {
                console.error('Double open detected for', uri);
                pendingOpens.delete(uri);
                didOpenEvent.clearTimeout();
                didOpenEvent.send();
            }

            const timerId = setTimeout(() => {
                pendingOpens.delete(uri);
                next(data);
            }, timeout);
            pendingOpens.set(uri, {
                send: () => { next(data); },
                clearTimeout: () => { clearTimeout(timerId); },
            });
        },
        didChange: (data, next) => {
            const uri = data.document.uri.toString();
            const didOpenEvent = pendingOpens.get(uri);
            if (didOpenEvent) {
                pendingOpens.delete(uri);
                didOpenEvent.clearTimeout();
                didOpenEvent.send();
            }
            next(data);
        },
        didClose: (data, next) => {
            const uri = data.uri.toString();
            const didOpenEvent = pendingOpens.get(uri);
            if (didOpenEvent) {
                pendingOpens.delete(uri);
                didOpenEvent.clearTimeout();
            }
            else
                next(data);
        },
    };
}
