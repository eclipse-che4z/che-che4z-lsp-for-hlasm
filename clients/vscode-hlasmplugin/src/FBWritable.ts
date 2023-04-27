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

import { Writable } from "stream";
import { TextDecoder } from "util";
import { convertBuffer } from "./conversions";

export class FBWritable extends Writable {
    private chunks: Buffer[] = [];
    private result: string = null;

    constructor(private lrecl: number = 80) { super(); }

    _write(chunk: Buffer, encoding: BufferEncoding, callback: (error?: Error | null) => void) {
        this.chunks.push(chunk);

        callback();
    }

    _final(callback: (error?: Error | null) => void) {
        const decoder = new TextDecoder();

        this.result = decoder.decode(convertBuffer(Buffer.concat(this.chunks), this.lrecl));

        callback();
    };

    getResult() { return this.result; }
}
