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

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/browser';
import { getConfig } from './eventsHandler';
import { ServerVariant, decorateArgs } from './serverFactory.common';
import { EXTENSION_ID } from './constants';

export async function createLanguageServer(_serverVariant: ServerVariant, clientOptions: vscodelc.LanguageClientOptions, extUri: vscode.Uri): Promise<vscodelc.BaseLanguageClient> {
    let extensionUri = extUri.toString();
    if (!extensionUri.endsWith('/'))
        extensionUri += '/';

    const worker = new Worker(extensionUri + 'bin/wasm/hlasm_language_server.js');
    worker.postMessage({ INIT: "INIT", extensionUri, arguments: decorateArgs(getConfig<string[]>('arguments', [])) });

    return new vscodelc.LanguageClient(EXTENSION_ID, 'HLASM extension Language Server', clientOptions, worker);
}
