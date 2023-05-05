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


import * as assert from 'assert';
import { HLASMExternalFilesFtp } from '../../hlasmExternalFilesFtp';
import { ExternalRequestType, HLASMExternalFiles } from '../../hlasmExternalFiles';
import { ExtensionContext } from 'vscode';

const extensionContextMock = undefined as any as ExtensionContext;

suite('External files (FTP)', () => {

    test('Dataset parsing', async () => {
        const ftpClient = new HLASMExternalFilesFtp(extensionContextMock);

        assert.strictEqual(ftpClient.parseArgs('aaa', ExternalRequestType.read_directory), null);
        assert.strictEqual(ftpClient.parseArgs('/0', ExternalRequestType.read_directory), null);
        assert.strictEqual(ftpClient.parseArgs('/AAAAAAAAA', ExternalRequestType.read_directory), null);
        assert.strictEqual(ftpClient.parseArgs('/AAAAAAAA/BBBBBBB', ExternalRequestType.read_directory), null);
        assert.strictEqual(ftpClient.parseArgs('/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAA.A', ExternalRequestType.read_directory), null);

        const full_length = ftpClient.parseArgs('/aaaaaaaa.aaaaaaaa.aaaaaaaa.aaaaaaaa.aaaaaaaa', ExternalRequestType.read_directory);
        assert.ok(full_length);
        assert.strictEqual(full_length.toString(), 'AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA');
        assert.strictEqual(full_length.normalizedPath(), '/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA/');
    });

    test('Dataset member parsing', async () => {
        const ftpClient = new HLASMExternalFilesFtp(extensionContextMock);

        assert.strictEqual(ftpClient.parseArgs('aaa', ExternalRequestType.read_file), null);
        assert.strictEqual(ftpClient.parseArgs('/0', ExternalRequestType.read_file), null);
        assert.strictEqual(ftpClient.parseArgs('/AAAAAAAAA', ExternalRequestType.read_file), null);
        assert.strictEqual(ftpClient.parseArgs('/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAA.A', ExternalRequestType.read_file), null);

        assert.strictEqual(ftpClient.parseArgs('/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA', ExternalRequestType.read_file), null);
        assert.strictEqual(ftpClient.parseArgs('/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA/0', ExternalRequestType.read_file), null);

        assert.strictEqual(ftpClient.parseArgs('/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA/BBBBBBBBB', ExternalRequestType.read_file), null);

        const full_length = ftpClient.parseArgs('/aaaaaaaa.aaaaaaaa.aaaaaaaa.aaaaaaaa.aaaaaaaa/bbbbbbbb', ExternalRequestType.read_file);
        assert.ok(full_length);
        assert.strictEqual(full_length.toString(), 'AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA(BBBBBBBB)');
        assert.strictEqual(full_length.normalizedPath(), '/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA/BBBBBBBB');
    });

    test('Suspend', async () => {
        const ftpClient = new HLASMExternalFilesFtp(extensionContextMock);

        assert.strictEqual(ftpClient.suspended(), false);

        const p = new Promise<boolean>(resolve => ftpClient.onStateChange(resolve));

        ftpClient.suspend();

        assert.strictEqual(await p, true);

        ftpClient.dispose();
    });

    test('Resume', async () => {
        const ftpClient = new HLASMExternalFilesFtp(extensionContextMock);
        ftpClient.suspend();

        assert.strictEqual(ftpClient.suspended(), true);

        const p = new Promise<boolean>(resolve => ftpClient.onStateChange(resolve));

        ftpClient.resume();

        assert.strictEqual(await p, false);

        ftpClient.dispose();
    });

    test('Invalid messages', async () => {
        const ext = new HLASMExternalFiles('test', {
            onNotification: (_, __) => { return { dispose: () => { } }; },
            sendNotification: (_: any, __: any) => Promise.resolve(),
        });

        assert.strictEqual(await ext.handleRawMessage(null), null);
        assert.strictEqual(await ext.handleRawMessage(undefined), null);
        assert.strictEqual(await ext.handleRawMessage({}), null);
        assert.strictEqual(await ext.handleRawMessage(5), null);
        assert.strictEqual(await ext.handleRawMessage({ id: 'id', op: '' }), null);
        assert.strictEqual(await ext.handleRawMessage({ id: 5, op: 5 }), null);

        assert.deepEqual(await ext.handleRawMessage({ id: 5, op: '' }), { id: 5, error: { code: -5, msg: 'Invalid request' } });
        assert.deepEqual(await ext.handleRawMessage({ id: 5, op: 'read_file', url: 5 }), { id: 5, error: { code: -5, msg: 'Invalid request' } });
        assert.deepEqual(await ext.handleRawMessage({ id: 5, op: 'read_file', url: {} }), { id: 5, error: { code: -5, msg: 'Invalid request' } });

        assert.deepEqual(await ext.handleRawMessage({ id: 5, op: 'read_file', url: 'unknown:scheme' }), { id: 5, error: { code: -5, msg: 'Invalid request' } });

        assert.deepEqual(await ext.handleRawMessage({ id: 5, op: 'read_file', url: 'test:/SERVICE' }), { id: 5, error: { code: -1000, msg: 'No client' } });
    });
});
