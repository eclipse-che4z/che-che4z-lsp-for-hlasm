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
import * as assert from 'assert';
import { HLASMExternalConfigurationProvider } from '../../hlasmExternalConfigurationProvider';

suite('External configuration provider', () => {
    test('Dispose', async () => {
        let disposeCalled = 0;
        const c = new HLASMExternalConfigurationProvider({
            onRequest: <R, E>(method: string, handler: vscodelc.GenericRequestHandler<R, E>): vscode.Disposable => {
                return { dispose: () => { ++disposeCalled; } };
            },
            sendNotification: async (method: string, params: any): Promise<void> => { }
        });

        const h = c.addHandler(async (uri) => null);

        assert.ok(h);
        assert.strictEqual(typeof h, 'object');

        c.dispose();

        assert.strictEqual(disposeCalled, 1);
    });

    test('Query after dispose', async () => {
        const c = new HLASMExternalConfigurationProvider({
            onRequest: <R, E>(method: string, handler: vscodelc.GenericRequestHandler<R, E>): vscode.Disposable => {
                return { dispose: () => { } };
            },
            sendNotification: async (method: string, params: any): Promise<void> => { }
        });

        const h = c.addHandler(async (uri) => { throw Error('Should not be called'); });

        assert.ok(h);
        assert.strictEqual(typeof h, 'object');

        h.dispose();

        const f = await c.handleRawMessage({ uri: '' });
        assert.ok('code' in f);
        assert.deepStrictEqual(f.code, 0);
    });

    test('Not found', async () => {
        const c = new HLASMExternalConfigurationProvider({
            onRequest: <R, E>(method: string, handler: vscodelc.GenericRequestHandler<R, E>): vscode.Disposable => {
                return { dispose: () => { } };
            },
            sendNotification: async (method: string, params: any): Promise<void> => { }
        });

        let calledWithUri: unknown;
        c.addHandler(async (uri) => {
            assert.ok(!calledWithUri);
            calledWithUri = uri;
            return null;
        });

        const f = await c.handleRawMessage({ uri: 'schema:path' });
        assert.ok('code' in f);
        assert.deepStrictEqual(f.code, 0);

        assert.ok(calledWithUri instanceof vscode.Uri);
        assert.deepStrictEqual(calledWithUri.toString(), 'schema:path');
    });

    test('Return configuration', async () => {
        const c = new HLASMExternalConfigurationProvider({
            onRequest: <R, E>(method: string, handler: vscodelc.GenericRequestHandler<R, E>): vscode.Disposable => {
                return { dispose: () => { } };
            },
            sendNotification: async (method: string, params: any): Promise<void> => { }
        });

        let calledWithUri: unknown;
        c.addHandler(async (uri) => {
            assert.ok(!calledWithUri);
            calledWithUri = uri;
            return { configuration: 'PROCGRP' };
        });

        const f = await c.handleRawMessage({ uri: 'schema:path' });
        assert.deepStrictEqual(f, { configuration: 'PROCGRP' });

        assert.ok(calledWithUri instanceof vscode.Uri);
        assert.deepStrictEqual(calledWithUri.toString(), 'schema:path');
    });

    test('Invalidation', async () => {
        let notificationParam: unknown;
        const c = new HLASMExternalConfigurationProvider({
            onRequest: <R, E>(method: string, handler: vscodelc.GenericRequestHandler<R, E>): vscode.Disposable => {
                return { dispose: () => { } };
            },
            sendNotification: async (method: string, params: any): Promise<void> => {
                assert.deepStrictEqual(method, 'invalidate_external_configuration');
                notificationParam = params;
            }
        });

        const h = c.addHandler(async (_) => null);

        await h.invalidate(null);
        assert.deepStrictEqual(notificationParam, {});

        await h.invalidate(vscode.Uri.parse('schema:path'));
        assert.deepStrictEqual(notificationParam, { uri: 'schema:path' });
    });

    test('Throwing handler', async () => {
        const c = new HLASMExternalConfigurationProvider({
            onRequest: <R, E>(method: string, handler: vscodelc.GenericRequestHandler<R, E>): vscode.Disposable => {
                return { dispose: () => { } };
            },
            sendNotification: async (method: string, params: any): Promise<void> => { }
        });

        const h = c.addHandler(async (_) => { throw Error('Error message') });

        const f = await c.handleRawMessage({ uri: 'schema:path' });
        assert.ok('code' in f && 'message' in f);
        assert.deepStrictEqual(f.code, -106);
        assert.deepStrictEqual(f.message, 'Error message');
    });
});
