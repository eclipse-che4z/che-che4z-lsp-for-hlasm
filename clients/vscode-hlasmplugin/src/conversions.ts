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

import { EOL } from 'os';
import { TextDecoder, TextEncoder } from 'util';

function toBufferArray(s: string): Uint8Array[] {
    const e = new TextEncoder();
    if (s.length != 256)
        throw Error("Single byte conversion table expected");
    const result: Uint8Array[] = [];
    for (const c of s)
        result.push(e.encode(c));
    return result;
}

const ibm1148WithCrlfReplacement = toBufferArray('\u0000\u0001\u0002\u0003\u009C\u0009\u0086\u007F\u0097\u008D\u008E\u000B\u000C\ue00D\u000E\u000F\u0010\u0011\u0012\u0013\u009D\ue025\u0008\u0087\u0018\u0019\u0092\u008F\u001C\u001D\u001E\u001F\u0080\u0081\u0082\u0083\u0084\u000A\u0017\u001B\u0088\u0089\u008A\u008B\u008C\u0005\u0006\u0007\u0090\u0091\u0016\u0093\u0094\u0095\u0096\u0004\u0098\u0099\u009A\u009B\u0014\u0015\u009E\u001A\u0020\u00A0\u00E2\u00E4\u00E0\u00E1\u00E3\u00E5\u00E7\u00F1\u005B\u002E\u003C\u0028\u002B\u0021\u0026\u00E9\u00EA\u00EB\u00E8\u00ED\u00EE\u00EF\u00EC\u00DF\u005D\u0024\u002A\u0029\u003B\u005E\u002D\u002F\u00C2\u00C4\u00C0\u00C1\u00C3\u00C5\u00C7\u00D1\u00A6\u002C\u0025\u005F\u003E\u003F\u00F8\u00C9\u00CA\u00CB\u00C8\u00CD\u00CE\u00CF\u00CC\u0060\u003A\u0023\u0040\u0027\u003D\u0022\u00D8\u0061\u0062\u0063\u0064\u0065\u0066\u0067\u0068\u0069\u00AB\u00BB\u00F0\u00FD\u00FE\u00B1\u00B0\u006A\u006B\u006C\u006D\u006E\u006F\u0070\u0071\u0072\u00AA\u00BA\u00E6\u00B8\u00C6\u20AC\u00B5\u007E\u0073\u0074\u0075\u0076\u0077\u0078\u0079\u007A\u00A1\u00BF\u00D0\u00DD\u00DE\u00AE\u00A2\u00A3\u00A5\u00B7\u00A9\u00A7\u00B6\u00BC\u00BD\u00BE\u00AC\u007C\u00AF\u00A8\u00B4\u00D7\u007B\u0041\u0042\u0043\u0044\u0045\u0046\u0047\u0048\u0049\u00AD\u00F4\u00F6\u00F2\u00F3\u00F5\u007D\u004A\u004B\u004C\u004D\u004E\u004F\u0050\u0051\u0052\u00B9\u00FB\u00FC\u00F9\u00FA\u00FF\u005C\u00F7\u0053\u0054\u0055\u0056\u0057\u0058\u0059\u005A\u00B2\u00D4\u00D6\u00D2\u00D3\u00D5\u0030\u0031\u0032\u0033\u0034\u0035\u0036\u0037\u0038\u0039\u00B3\u00DB\u00DC\u00D9\u00DA\u009F');

export function convertBuffer(buffer: Uint8Array, lrecl: number) {
    const EOLBuffer = new TextEncoder().encode(EOL);
    // 0xe000 private plane has 3 byte encoding sequence
    const result = new Uint8Array(3 * buffer.length + Math.floor((buffer.length + lrecl - 1) / lrecl) * EOLBuffer.length);
    let pos = 0;
    let i = 0;
    for (const v of buffer) {
        result.set(ibm1148WithCrlfReplacement[v], pos);
        pos += ibm1148WithCrlfReplacement[v].length;
        if (i % lrecl === lrecl - 1) {
            result.set(EOLBuffer, pos);
            pos += EOLBuffer.length;
        }
        ++i;
    }
    return result.subarray(0, pos);
}

const uriFriendlyBase16String = 'abcdefghijklmnop';
const uriFriendlyBase16StringBoth = uriFriendlyBase16String + uriFriendlyBase16String.toUpperCase();

const uriFriendlyBase16Map = (() => {
    const result = [];
    for (const c0 of uriFriendlyBase16String)
        for (const c1 of uriFriendlyBase16String)
            result.push(c0 + c1);

    return result;
})();

export function uriFriendlyBase16Encode(s: string) {
    return [...new TextEncoder().encode(s)].map(x => uriFriendlyBase16Map[x]).join('');
}

export function uriFriendlyBase16Decode(s: string) {
    if (s.length & 1) return '';
    const array = [];
    for (let i = 0; i < s.length; i += 2) {
        const c0 = uriFriendlyBase16StringBoth.indexOf(s[i]);
        const c1 = uriFriendlyBase16StringBoth.indexOf(s[i + 1]);
        if (c0 < 0 || c1 < 0) return '';
        array.push((c0 & 15) << 4 | (c1 & 15));
    }
    try {
        return new TextDecoder(undefined, { fatal: true, ignoreBOM: false }).decode(Uint8Array.from(array));
    } catch (e) {
        return '';
    }
}
