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

import { textEncode, textDecode } from './tools.common';

export const isWeb = true;

function makeReadable(data: Uint8Array) {
    return new ReadableStream({
        start(controller) {
            controller.enqueue(data);
            controller.close();
        },
    });
}

export async function deflate(data: Uint8Array): Promise<Uint8Array> {
    // @ts-ignore
    return new Uint8Array(await new Response(makeReadable(data).pipeThrough(new CompressionStream('deflate'))).arrayBuffer());
}

export async function inflate(data: Uint8Array): Promise<Uint8Array> {
    // @ts-ignore
    return new Uint8Array(await new Response(makeReadable(data).pipeThrough(new DecompressionStream('deflate'))).arrayBuffer());
}

export const EOL = '\n';
const hexNumbers = '0123456789abcdef0123456789ABCDEF';

export async function sha256(s: string): Promise<string> {
    // This will fail outside of secure contexts because of W3C.

    const hash = await crypto.subtle.digest("SHA-256", textEncode(s));

    let result = '';
    for (const b of new Uint8Array(hash)) {
        result += hexNumbers[b >> 4];
        result += hexNumbers[b & 15];
    }

    return result;
}

export function decodeBase64(s: string): string {
    return self.atob(s);
}

export function textFromHex(s: string): string {
    return textDecode(arrayFromHex(s));
}

export function arrayFromHex(s: string): Uint8Array {
    const result = new Uint8Array(s.length / 2);
    for (let i = 0; i < s.length / 2; ++i) {
        const u = hexNumbers.indexOf(s[2 * i]) % 16;
        const l = hexNumbers.indexOf(s[2 * i + 1]) % 16;
        if (u < 0 || l < 0)
            return result.slice(0, i);

        result[i] = u * 16 + l;
    }
    return result;
}
