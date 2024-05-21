/*
 * Copyright (c) 2024 Broadcom.
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
import { schemaConfiguration } from './constants';

export function registerConfigurationFileSystemProvider(extensionUri: vscode.Uri) {
    const generatePath = (uri: vscode.Uri) => vscode.Uri.joinPath(extensionUri, uri.path);

    const emmiter = new vscode.EventEmitter<vscode.FileChangeEvent[]>();
    vscode.workspace.registerFileSystemProvider(schemaConfiguration, {
        onDidChangeFile: emmiter.event,
        watch() { return { dispose: () => { } }; },
        stat(uri) { return vscode.workspace.fs.stat(generatePath(uri)); },
        readDirectory() { throw Error("not implemented"); },
        createDirectory() { throw Error("not implemented"); },
        readFile(uri) { return vscode.workspace.fs.readFile(generatePath(uri)); },
        writeFile() { throw Error("not implemented"); },
        delete() { throw Error("not implemented"); },
        rename() { throw Error("not implemented"); },
    }, { isReadonly: true, isCaseSensitive: true });

}
