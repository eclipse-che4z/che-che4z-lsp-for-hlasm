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
import { ConnectionPool } from '../../connectionPool';
import { sleep } from './testHelper';

function makeTestFactory() {
    return {
        createdCount: 0,
        reusableCount: 0,
        closeCount: 0,

        create: function () {
            ++this.createdCount;
            return { closed: false, reusable: true };
        },
        reusable: function (c: ReturnType<typeof this.create>) {
            ++this.reusableCount;

            return !c.closed && c.reusable;
        },
        close: function (c: ReturnType<typeof this.create>) {
            ++this.closeCount;

            assert.strictEqual(c.closed, false);

            c.closed = true;
        }
    }
}
const inf = 1000000;

suite('Connection pool', () => {
    test('Basic reuse', async () => {
        const f = makeTestFactory();

        const cp = new ConnectionPool(f, 1, inf);

        assert.strictEqual(f.createdCount, 0);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 0);

        let firstClient: unknown = null;

        let signalResolve = () => { };
        const signal = new Promise<void>(r => signalResolve = r);
        let waitForResolve = () => { };
        const waitFor = new Promise<void>(r => waitForResolve = r);

        const p1 = cp.withClient(async (c) => {
            firstClient = c;
            signalResolve();
            await waitFor;
        });

        await signal;

        const p2 = cp.withClient(async (c) => {
            assert.strictEqual(c, firstClient);
        });

        await Promise.resolve();

        assert.strictEqual(f.createdCount, 1);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 0);

        waitForResolve();

        await Promise.allSettled([p1, p2]);

        assert.strictEqual(f.createdCount, 1);
        assert.strictEqual(f.reusableCount, 2);
        assert.strictEqual(f.closeCount, 0);

        cp.dispose();

        assert.strictEqual(f.createdCount, 1);
        assert.strictEqual(f.reusableCount, 2);
        assert.strictEqual(f.closeCount, 1);
    });

    test('Concurent use', async () => {
        const f = makeTestFactory();

        const cp = new ConnectionPool(f, 2, inf);

        assert.strictEqual(f.createdCount, 0);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 0);

        let firstClient: unknown = null;

        let signalResolve = () => { };
        const signal = new Promise<void>(r => signalResolve = r);
        let signal2Resolve = () => { };
        const signal2 = new Promise<void>(r => signal2Resolve = r);
        let waitForResolve = () => { };
        const waitFor = new Promise<void>(r => waitForResolve = r);

        const p1 = cp.withClient(async (c) => {
            firstClient = c;
            signalResolve();
            await waitFor;
        });

        const p2 = cp.withClient(async (c) => {
            assert.notStrictEqual(c, firstClient);
            signal2Resolve();
            await waitFor;
        });

        await signal;
        await signal2;

        assert.strictEqual(f.createdCount, 2);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 0);

        waitForResolve();

        await Promise.allSettled([p1, p2]);

        cp.dispose();

        assert.strictEqual(f.createdCount, 2);
        assert.strictEqual(f.reusableCount, 2);
        assert.strictEqual(f.closeCount, 2);
    });

    test('Pool timout', async () => {
        const f = makeTestFactory();
        const timeoutLimit = 1;
        const cp = new ConnectionPool(f, 1, timeoutLimit);

        assert.strictEqual(f.createdCount, 0);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 0);

        await cp.withClient(async (c) => { });

        assert.strictEqual(f.createdCount, 1);
        assert.strictEqual(f.reusableCount, 1);
        assert.strictEqual(f.closeCount, 0);

        await sleep(timeoutLimit + 1);

        assert.strictEqual(f.createdCount, 1);
        assert.strictEqual(f.reusableCount, 1);
        assert.strictEqual(f.closeCount, 1);

        cp.dispose();

        assert.strictEqual(f.createdCount, 1);
        assert.strictEqual(f.reusableCount, 1);
        assert.strictEqual(f.closeCount, 1);
    });

    test('Action fail', async () => {
        const f = makeTestFactory();
        const cp = new ConnectionPool(f, 1, inf);

        assert.strictEqual(f.createdCount, 0);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 0);

        const actionFailed = 'Action failed';
        try {
            await cp.withClient(async (c) => { throw Error(actionFailed) });
            assert.ok(false);
        }
        catch (e) {
            assert.ok(e instanceof Error);
            assert.strictEqual(e.message, actionFailed);
        }

        assert.strictEqual(f.createdCount, 1);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 1);

        cp.dispose();

        assert.strictEqual(f.createdCount, 1);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 1);
    });

    test('Non-reusable client', async () => {
        const f = makeTestFactory();
        const cp = new ConnectionPool(f, 1, inf);

        assert.strictEqual(f.createdCount, 0);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 0);

        await cp.withClient(async (c) => { c.reusable = false; });

        assert.strictEqual(f.createdCount, 1);
        assert.strictEqual(f.reusableCount, 1);
        assert.strictEqual(f.closeCount, 1);

        cp.dispose();
    });

    test('Generation up', async () => {
        const f = makeTestFactory();
        const cp = new ConnectionPool(f, 1, inf);

        assert.strictEqual(f.createdCount, 0);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 0);

        let waitForResolve = () => { };
        const waitFor = new Promise<void>(r => waitForResolve = r);
        let signalResolve = () => { };
        const signal = new Promise<void>(r => signalResolve = r);

        let firstClinet: unknown;
        const p = cp.withClient(async (c) => {
            firstClinet = c;
            signalResolve();
            await waitFor;
            const a = {};
        });
        await signal;
        cp.closeClients();

        waitForResolve();

        await p;

        assert.strictEqual(f.createdCount, 1);
        assert.strictEqual(f.reusableCount, 0);
        assert.strictEqual(f.closeCount, 1);

        await cp.withClient(c => { assert.notStrictEqual(c, firstClinet); });

        assert.strictEqual(f.createdCount, 2);
        assert.strictEqual(f.reusableCount, 1);
        assert.strictEqual(f.closeCount, 1);

        cp.dispose();

        assert.strictEqual(f.createdCount, 2);
        assert.strictEqual(f.reusableCount, 1);
        assert.strictEqual(f.closeCount, 2);
    });
});
