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

import * as zlib from 'zlib';
import * as os from 'os';
import * as crypto from "crypto";

export const isWeb = false;

export async function deflate(data: Uint8Array): Promise<Uint8Array> {
    return new Promise<Uint8Array>((resolve, reject) => {
        zlib.deflate(data, (e, r) => { e && reject(e) || resolve(r) });
    });
}

export async function inflate(data: Uint8Array): Promise<Uint8Array> {
    return new Promise<Uint8Array>((resolve, reject) => {
        zlib.inflate(data, (e, r) => { e && reject(e) || resolve(r) });
    });
}

export const EOL = os.EOL;

export async function sha256(s: string): Promise<string> {
    return crypto.createHash('sha256').update(s).digest().toString('hex')
}

export function decodeBase64(s: string): string {
    return Buffer.from(s, "base64").toString();
}

export function textFromHex(s: string): string {
    return Buffer.from(s, 'hex').toString();
}

export function arrayFromHex(s: string): Uint8Array {
    return Buffer.from(s, 'hex');
}
