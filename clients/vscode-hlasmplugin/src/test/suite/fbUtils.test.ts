/*
 * Copyright (c) 2024 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of FB the Eclipse Public License 2.0
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
import { FBStreamingConvertor } from "../../FBStreamingConvertor";

suite('FB Utilities', () => {

    test('Convertor', async () => {
        const convertor = new FBWritable(10);
        const input = Buffer.from([0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2]);

        await Stream.promises.pipeline(Readable.from(input), convertor);

        assert.deepStrictEqual(convertor.getResult().split(/\r?\n/), ["AAAAAAAAAA", "BBBBBBBBBB", ""]);
    });

    test('Streaming Convertor', async () => {
        const convertor = new FBStreamingConvertor(10);

        convertor.write(Uint8Array.from([0xc1, 0xc1, 0xc1, 0xc1, 0xc1,]));
        convertor.write(Uint8Array.from([0xc1, 0xc1, 0xc1, 0xc1, 0xc1, 0xc2, 0xc2, 0xc2, 0xc2,]));
        convertor.write(Uint8Array.from([0xc2, 0xc2, 0xc2, 0xc2, 0xc2, 0xc2]));

        assert.deepStrictEqual(convertor.getResult().split(/\r?\n/), ["AAAAAAAAAA", "BBBBBBBBBB", ""]);
        // subsequent call should return the same content
        assert.deepStrictEqual(convertor.getResult().split(/\r?\n/), ["AAAAAAAAAA", "BBBBBBBBBB", ""]);
    });

});
