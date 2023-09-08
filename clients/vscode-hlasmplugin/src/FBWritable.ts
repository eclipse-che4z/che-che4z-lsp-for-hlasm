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
import { convertBuffer } from "./conversions";
import { concat } from "./helpers";
import { textDecode } from "./tools.common";

export class FBWritable extends Writable {
    private chunks: Uint8Array[] = [];
    private result?: string;

    constructor(private lrecl: number = 80) { super(); }

    _write(chunk: Buffer, encoding: BufferEncoding, callback: (error?: Error | null) => void) {
        this.chunks.push(chunk);

        callback();
    }

    _final(callback: (error?: Error | null) => void) {
        this.result = textDecode(convertBuffer(concat(...this.chunks), this.lrecl));

        callback();
    };

    getResult() { return this.result!; }
}
