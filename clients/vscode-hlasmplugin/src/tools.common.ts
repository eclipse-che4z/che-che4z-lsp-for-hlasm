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

const decoder = new TextDecoder();
const decoderStrict = new TextDecoder(undefined, { fatal: true, ignoreBOM: true });
const encoder = new TextEncoder();

export function textDecode(input: Uint8Array): string {
    return decoder.decode(input);
}

export function textDecodeStrict(input: Uint8Array): string {
    return decoderStrict.decode(input);
}

export function textEncode(input: string) {
    return encoder.encode(input);
}

export function stripJsonComments(input: string): string {
    const ar = input.split('');

    let state = 0;
    // 0 - normal
    // 1 - string
    // 2 - /*-comment
    // 3 - //-comment
    for (let i = 0; i < ar.length; ++i) {
        const c = ar[i];
        switch (state) {
            case 0:
                if (c === '"') {
                    state = 1;
                }
                else if (c === '/' && ar[i + 1] === '*') {
                    ar[i + 1] = ' '; // pre-clear to handle /*/
                    state = 2;
                }
                else if (c === '/' && ar[i + 1] === '/') {
                    state = 3;
                }
                break;
            case 1:
                if (c === '\\' && (ar[i + 1] === '"' || ar[i + 1] === '\\')) {
                    ++i;
                }
                else if (c === '"') {
                    state = 0;
                }
                break;
            case 2:
                if (c === '*' && ar[i + 1] === '/') {
                    ar[i] = ' ';
                    ar[i + 1] = ' ';
                    ++i;
                    state = 0;
                }
                break;
            case 3:
                if (c === '\n' || c === '\r') {
                    state = 0;
                }
                break;
        }
        if (state >= 2)
            ar[i] = ' ';
    }

    return ar.join('');
}
