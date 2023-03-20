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

    const sendOpenNotification = (uri: string) => {
        const didOpenEvent = pendingOpens.get(uri);
        if (didOpenEvent) {
            pendingOpens.delete(uri);
            didOpenEvent.clearTimeout();
            didOpenEvent.send();

            return true;
        }
        return false;
    };

    return {
        didOpen: (data, next) => {
            const uri = data.uri.toString();

            sendOpenNotification(uri) && console.error('Double open detected for', uri);

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
            sendOpenNotification(data.document.uri.toString());
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
