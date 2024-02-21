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
import { isCancellationError } from "../../helpers";
import { CancellationError } from "vscode";
import { textFromHex, arrayFromHex } from '../../tools.web';

suite('Utilities', () => {

    test('Cancellation error detection', () => {
        assert.ok(isCancellationError(new CancellationError()));
        assert.ok(isCancellationError(new Error("Canceled")));
        assert.ok(!isCancellationError(new Error("Something")));
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
