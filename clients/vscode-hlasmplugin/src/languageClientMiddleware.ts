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

export type HlasmPluginMiddleware = Middleware & {
    getFirstOpen: () => Promise<void>,
    resetFirstOpen: () => void,
};

export function getLanguageClientMiddleware(): HlasmPluginMiddleware {
    // VSCode generates flood of didOpen/didClose event when trying to go to a symbol definition using mouse (CTRL+point).
    // This behavior is documented (and apprently work on) but it causes meaningless activity in the language server.

    // The purpose of the middleware is to delay sending the didOpen event by a fraction of a second
    // and attempt to pair it with didClose generated almost immediatelly after.
    // In that case, we drop both events.

    const pendingOpens = new Map<string, {
        send: () => boolean,
        forget: () => boolean,
    }>();
    const timeout = 50;

    const sendOpenNotification = (uri: string) => pendingOpens.get(uri)?.send() || false;

    let resolveFirstOpen: undefined | (() => void);
    let firstOpen = new Promise<void>((res, _) => { resolveFirstOpen = res });

    return {
        getFirstOpen: () => {
            return firstOpen;
        },
        resetFirstOpen: () => {
            if (!resolveFirstOpen)
                firstOpen = new Promise<void>((res, _) => { resolveFirstOpen = res });
        },
        didOpen: (data, next) => {
            return new Promise<void>((resolve, reject) => {
                const uri = data.uri.toString();

                sendOpenNotification(uri) && console.error('Double open detected for', uri);

                const deleteAndSend = () => {
                    pendingOpens.delete(uri);
                    next(data).then(resolve, reject);
                    if (resolveFirstOpen) {
                        resolveFirstOpen();
                        resolveFirstOpen = undefined;
                    }
                };

                const timerId = setTimeout(deleteAndSend, timeout);

                pendingOpens.set(uri, {
                    send: () => {
                        clearTimeout(timerId);
                        deleteAndSend();
                        return true;
                    },
                    forget: () => {
                        clearTimeout(timerId);
                        pendingOpens.delete(uri);
                        resolve();
                        return true;
                    },
                });
            });
        },
        didChange: (data, next) => {
            sendOpenNotification(data.document.uri.toString());
            return next(data);
        },
        didClose: (data, next) => {
            return pendingOpens.get(data.uri.toString())?.forget()
                ? Promise.resolve()
                : next(data);
        },
        provideDefinition: (document, position, token, next) => {
            sendOpenNotification(document.uri.toString());
            return next(document, position, token);
        },
        provideReferences: (document, position, token, options, next) => {
            sendOpenNotification(document.uri.toString());
            return next(document, position, token, options);
        },
        provideHover: (document, position, token, next) => {
            sendOpenNotification(document.uri.toString());
            return next(document, position, token);
        },
        provideCompletionItem: (document, position, context, token, next) => {
            sendOpenNotification(document.uri.toString());
            return next(document, position, context, token);
        },
        provideDocumentSemanticTokens: (document, token, next) => {
            sendOpenNotification(document.uri.toString());
            return next(document, token);
        },
        provideDocumentSymbols: (document, token, next) => {
            sendOpenNotification(document.uri.toString());
            return next(document, token);
        },
    };
}
