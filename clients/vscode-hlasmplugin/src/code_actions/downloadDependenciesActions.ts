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

export function generateDownloadDependenciesCodeActions(): vscode.CodeAction[] {
    const result: vscode.CodeAction[] = [];

    result.push({
        title: 'Download missing dependencies',
        command: {
            title: 'Download dependencies',
            command: 'extension.hlasm-plugin.downloadDependencies',
            arguments: ['newOnly']
        },
        kind: vscode.CodeActionKind.QuickFix
    });
    result.push({
        title: 'Download all dependencies',
        command: {
            title: 'Download dependencies',
            command: 'extension.hlasm-plugin.downloadDependencies'
        },
        kind: vscode.CodeActionKind.QuickFix
    });

    return result;
}
