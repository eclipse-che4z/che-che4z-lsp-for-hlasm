/*
 * Copyright (c) 2024 Broadcom.
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
import { AsyncMutex, AsyncSemaphore } from "../../asyncMutex";

suite('Async mutex', () => {

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

    test('Semaphore argument validation', (done) => {
        try { new AsyncSemaphore(0); done(Error("Failed")); } catch (e) { }
        try { new AsyncSemaphore(-1); done(Error("Failed")); } catch (e) { }
        try { new AsyncSemaphore(<any>null); done(Error("Failed")); } catch (e) { }
        try { new AsyncSemaphore(<any>undefined); done(Error("Failed")); } catch (e) { }

        done();
    });

});
