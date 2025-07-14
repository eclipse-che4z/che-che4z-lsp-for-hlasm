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
import { HLASMExternalFilesMF } from '../../hlasmExternalFilesMF';
import { ExternalRequestType } from '../../hlasmExternalFiles';
import { ExtensionContext } from 'vscode';

const extensionContextMock = undefined as any as ExtensionContext;

suite('External files (MF)', () => {

    test('Dataset parsing', async () => {
        const client = HLASMExternalFilesMF(extensionContextMock);

        assert.strictEqual(await client.parseArgs('aaa', ExternalRequestType.list_directory), null);
        assert.strictEqual(await client.parseArgs('/0', ExternalRequestType.list_directory), null);
        assert.strictEqual(await client.parseArgs('/AAAAAAAAA', ExternalRequestType.list_directory), null);
        assert.strictEqual(await client.parseArgs('/AAAAAAAA/BBBBBBB', ExternalRequestType.list_directory), null);
        assert.strictEqual(await client.parseArgs('/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAA.A', ExternalRequestType.list_directory), null);

        const full_length = await client.parseArgs('/aaaaaaaa.aaaaaaaa.aaaaaaaa.aaaaaaaa.aaaaaaaa', ExternalRequestType.list_directory);
        assert.ok(full_length);
        assert.strictEqual(full_length.details.toDisplayString(), 'AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA');
        assert.strictEqual(full_length.details.normalizedPath(), '/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA/');
        assert.strictEqual(full_length.server, undefined);
    });

    test('Dataset member parsing', async () => {
        const client = HLASMExternalFilesMF(extensionContextMock);

        assert.strictEqual(await client.parseArgs('aaa', ExternalRequestType.read_file), null);
        assert.strictEqual(await client.parseArgs('/0', ExternalRequestType.read_file), null);
        assert.strictEqual(await client.parseArgs('/AAAAAAAAA', ExternalRequestType.read_file), null);
        assert.strictEqual(await client.parseArgs('/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAA.A', ExternalRequestType.read_file), null);

        assert.strictEqual(await client.parseArgs('/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA', ExternalRequestType.read_file), null);
        assert.strictEqual(await client.parseArgs('/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA/0', ExternalRequestType.read_file), null);

        assert.strictEqual(await client.parseArgs('/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA/BBBBBBBBB', ExternalRequestType.read_file), null);

        const full_length = await client.parseArgs('/aaaaaaaa.aaaaaaaa.aaaaaaaa.aaaaaaaa.aaaaaaaa/bbbbbbbb', ExternalRequestType.read_file);
        assert.ok(full_length);
        assert.strictEqual(full_length.details.toDisplayString(), 'AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA(BBBBBBBB)');
        assert.strictEqual(full_length.details.normalizedPath(), '/AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA.AAAAAAAA/BBBBBBBB');
        assert.strictEqual(full_length.server, undefined);
    });
});
