/*
 * Copyright (c) 2022 Broadcom.
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
import * as vscode from 'vscode';

export async function uriExists(uri: vscode.Uri, fs: vscode.FileSystem = vscode.workspace.fs): Promise<boolean> {
    return fs.stat(uri).then(() => { return true; }, () => { return false; })
}

export function isCancellationError(e: any) {
    return e instanceof vscode.CancellationError || e instanceof Error && e.message == new vscode.CancellationError().message;
}

export function asError(x: unknown) {
    return x instanceof Error ? x : Error('Unknown error:' + x);
}

export function concat(...is: Uint8Array[]) {
    const result = new Uint8Array(is.reduce((t, b) => t + b.length, 0));
    let idx = 0;
    for (const i of is) {
        result.set(i, idx);
        idx += i.length;
    }
    return result;
}
