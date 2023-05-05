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
import * as vscodelc from 'vscode-languageclient';

interface FileContent {
    content: string;
}

export class HLASMVirtualFileContentProvider implements vscode.TextDocumentContentProvider {
    onDidChange?: vscode.Event<vscode.Uri> = undefined;
    provideTextDocumentContent(uri: vscode.Uri, token: vscode.CancellationToken): vscode.ProviderResult<string> {
        return new Promise((resolve, reject) => {
            const trimmed = uri.authority.trim();
            const file_id = +trimmed;
            if (uri.scheme === 'hlasm' && trimmed.length > 0 && !isNaN(file_id))
                this.client.sendRequest<FileContent>("get_virtual_file_content", { id: file_id }, token)
                    .then(c => resolve(c.content))
                    .catch(e => reject(e));

            else
                reject("Invalid virtual HLASM file.");
        });
    }

    constructor(private client: vscodelc.BaseLanguageClient) {
    }
}
