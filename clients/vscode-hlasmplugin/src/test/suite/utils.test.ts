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

import { Readable, Stream } from "stream";
import { FBWritable } from "../../FBWritable";
import * as assert from 'assert';
import { AsyncMutex, AsyncSemaphore } from "../../asyncMutex";
import { isCancellationError } from "../../helpers";
import { CancellationError } from "vscode";
import { connectionSecurityLevel, gatherSecurityLevelFromZowe, translateConnectionInfo } from "../../ftpCreds";
import { FBStreamingConvertor } from "../../FBStreamingConvertor";
import { textFromHex, arrayFromHex } from '../../tools.web';

suite('Utilities', () => {

    test('FB Convertor', async () => {
        const convertor = new FBWritable(10);
        const input = Buffer.from([0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2]);

        await Stream.promises.pipeline(Readable.from(input), convertor);

        assert.deepStrictEqual(convertor.getResult().split(/\r?\n/), ["AAAAAAAAAA", "BBBBBBBBBB", ""]);
    });

    test('FB Streaming Convertor', async () => {
        const convertor = new FBStreamingConvertor(10);

        convertor.write(Uint8Array.from([0xc1, 0xc1, 0xc1, 0xc1, 0xc1,]));
        convertor.write(Uint8Array.from([0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc2, 0xc2, 0xc2, 0xc2,]));
        convertor.write(Uint8Array.from([0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2]));

        assert.deepStrictEqual(convertor.getResult().split(/\r?\n/), ["AAAAAAAAAA", "BBBBBBBBBB", ""]);
        // subsequent call should return the same content
        assert.deepStrictEqual(convertor.getResult().split(/\r?\n/), ["AAAAAAAAAA", "BBBBBBBBBB", ""]);
    });

    test('Async mutex', async () => {
        const mutex = new AsyncMutex();

        let i = 0;

        let wakeupCallback = () => { };
        const wakeup = new Promise<void>(r => { wakeupCallback = r; });

        const a1 = mutex.locked(async () => {
            assert.strictEqual(i, 0);

            await wakeup;

            assert.strictEqual(i, 0);

            i = 1;
        });

        const a2 = mutex.locked(async () => {
            assert.strictEqual(i, 1);
        });

        wakeupCallback();

        await Promise.all([a1, a2]);

    }).timeout(10000).slow(2000);

    test('Async semaphore', async () => {
        const sem = new AsyncSemaphore(2);

        let i = 0;

        let wakeupCallback = () => { };
        const wakeup = new Promise<void>(r => { wakeupCallback = r; });

        const a1 = sem.locked(async () => {
            assert.strictEqual(i, 0);

            await wakeup;

            assert.notStrictEqual(i, 2);

            ++i;
        });

        const a2 = sem.locked(async () => {
            assert.strictEqual(i, 0);

            await wakeup;

            assert.notStrictEqual(i, 2);

            ++i;
        });

        const a3 = sem.locked(async () => {
            assert.notStrictEqual(i, 0);
        });

        wakeupCallback();

        await Promise.all([a1, a2, a3]);

        assert.strictEqual(i, 2);

    }).timeout(10000).slow(2000);

    test('Cancellation error detection', () => {
        assert.ok(isCancellationError(new CancellationError()));
        assert.ok(isCancellationError(new Error("Canceled")));
        assert.ok(!isCancellationError(new Error("Something")));
    });

    test('Semaphore argument validation', (done) => {
        try { new AsyncSemaphore(0); done(Error("Failed")); } catch (e) { }
        try { new AsyncSemaphore(-1); done(Error("Failed")); } catch (e) { }
        try { new AsyncSemaphore(<any>null); done(Error("Failed")); } catch (e) { }
        try { new AsyncSemaphore(<any>undefined); done(Error("Failed")); } catch (e) { }

        done();
    });

    test('zowe profile translation', () => {
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: false }), connectionSecurityLevel.unsecure);

        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({}), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: '' }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: 0 }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({}), connectionSecurityLevel.rejectUnauthorized);

        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true, rejectUnauthorized: true }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true, rejectUnauthorized: '' }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true, rejectUnauthorized: 0 }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true, rejectUnauthorized: '' }), connectionSecurityLevel.rejectUnauthorized);

        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true, rejectUnauthorized: false }), connectionSecurityLevel.acceptUnauthorized);
    });

    test('Connection info translation', () => {
        assert.deepStrictEqual(translateConnectionInfo({
            host: 'h',
            port: 12345,
            user: 'u',
            password: 'p',
            securityLevel: connectionSecurityLevel.unsecure
        }), {
            host: 'h',
            user: 'u',
            password: 'p',
            port: 12345,
            secure: false,
            secureOptions: undefined
        });
        assert.deepStrictEqual(translateConnectionInfo({
            host: 'h',
            port: 12345,
            user: 'u',
            password: 'p',
            securityLevel: connectionSecurityLevel.acceptUnauthorized
        }), {
            host: 'h',
            user: 'u',
            password: 'p',
            port: 12345,
            secure: true,
            secureOptions: { rejectUnauthorized: false }
        });
        assert.deepStrictEqual(translateConnectionInfo({
            host: 'h',
            port: 12345,
            user: 'u',
            password: 'p',
            securityLevel: connectionSecurityLevel.rejectUnauthorized
        }), {
            host: 'h',
            user: 'u',
            password: 'p',
            port: 12345,
            secure: true,
            secureOptions: { rejectUnauthorized: true }
        });
    });

    test('Tools web alternative', () => {
        let allBytesText = '';
        for (const u of '0123456789abcdef')
            for (const l of '0123456789ABCDEF')
                allBytesText += u + l;
        const allBytes = arrayFromHex(allBytesText);
        for (let i = 0; i < 256; ++i)
            assert.strictEqual(allBytes.at(i), i);

        assert.strictEqual(textFromHex('48656c6c6f2c20576f726c6421'), 'Hello, World!');
    });
});
