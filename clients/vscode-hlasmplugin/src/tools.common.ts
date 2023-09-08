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

export function textEncode(input: string): Uint8Array {
    return encoder.encode(input);
}
