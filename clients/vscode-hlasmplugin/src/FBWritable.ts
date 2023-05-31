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
import { concat } from "./helpers";

export class FBWritable extends Writable {
    private chunks: Uint8Array[] = [];
    private result?: string;

    constructor(private lrecl: number = 80) { super(); }

    _write(chunk: Buffer, encoding: BufferEncoding, callback: (error?: Error | null) => void) {
        this.chunks.push(chunk);

        callback();
    }

    _final(callback: (error?: Error | null) => void) {
        const decoder = new TextDecoder();

        this.result = decoder.decode(convertBuffer(concat(...this.chunks), this.lrecl));

        callback();
    };

    getResult() { return this.result!; }
}

const decoder = new TextDecoder();
export class FBStreamingConvertor {
    private pending: Uint8Array = new Uint8Array();
    private resultChunks: string[] = [];

    constructor(private lrecl: number = 80) { }

    private processBlock(b: Uint8Array) {
        this.resultChunks.push(decoder.decode(convertBuffer(b, this.lrecl)));
    }

    write(chunk: Uint8Array) {
        if (this.pending.length > 0 && this.pending.length + chunk.length >= this.lrecl) {
            const missing = this.lrecl - this.pending.length;
            this.processBlock(concat(this.pending, chunk.subarray(0, missing)));
            chunk = chunk.subarray(missing);
        }
        const use = chunk.length - chunk.length % this.lrecl;
        this.processBlock(chunk.subarray(0, use));
        this.pending = chunk.slice(use); // make copy!!!
    }

    getResult() {
        if (this.pending.length > 0) {
            this.processBlock(this.pending);
            this.pending = new Uint8Array();
        }

        if (this.resultChunks.length !== 1)
            this.resultChunks = [this.resultChunks.join('')];
        return this.resultChunks[0];
    }
}
