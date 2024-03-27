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
import * as vscodelc from 'vscode-languageclient';
import { uriFriendlyBase16Decode, uriFriendlyBase16Encode } from './conversions';
import { isCancellationError } from './helpers';
import { pickUser } from './uiUtils';

type OutputLine = {
    level: number;
    text: string;
};
type OutputResult = OutputLine[];

type Options = { mnote: true, punch: true } | { mnote: false, punch: true } | { mnote: true, punch: false };

const scheme = 'hlasm-output';

function translateOptions(options: Options) {
    let result = '';
    if (options.mnote) result += 'M';
    if (options.punch) result += 'P';
    return result;
}

function extractOptions(uri: vscode.Uri): Options {
    const opts = uri.path.split('/')[1];
    if (!opts) throw Error('Invalid URI format');

    const result = { mnote: opts.indexOf('M') >= 0, punch: opts.indexOf('P') >= 0, };
    if (!result.mnote && !result.punch) throw Error('Invalid output options');

    return result as Options;
}

function createOutputUri(uri: vscode.Uri, options: Options) {
    const query = uriFriendlyBase16Encode(uri.toString());
    const path = `/${translateOptions(options)}/${uri.path.substring(uri.path.lastIndexOf('/') + 1)}.output`;

    return vscode.Uri.from({ scheme, path, query });
}

export async function showOutputCommand(editor: vscode.TextEditor, _: vscode.TextEditorEdit, args: any) {
    if (editor.document.languageId !== 'hlasm') return;
    if (!args || args instanceof vscode.Uri)
        args = await pickUser('Outputs to include:', [
            { label: 'MNOTE and PUNCH', value: { mnote: true, punch: true } },
            { label: 'MNOTE only', value: { mnote: true, punch: false } },
            { label: 'PUNCH only', value: { mnote: false, punch: true } },
        ]);
    if (isCancellationError(args)) return;
    return vscode.workspace.openTextDocument(createOutputUri(editor.document.uri, args)).then(
        doc => vscode.window.showTextDocument(doc, { viewColumn: vscode.ViewColumn.Beside, preserveFocus: true })
    );
}

function outputLineToString(l: OutputLine) {
    if (l.level < 0)
        return l.text;
    else
        return `${String(l.level).padStart(3, '0')}:${l.text}`;
}

export function registerOutputDocumentContentProvider(
    channel: {
        onNotification(method: string, handler: vscodelc.GenericNotificationHandler): vscode.Disposable;
        sendRequest<R>(method: string, param: any, token?: vscode.CancellationToken): Promise<R>;
    }, disposables: vscode.Disposable[]) {

    const changed = new vscode.EventEmitter<vscode.Uri>()
    const provider = vscode.workspace.registerTextDocumentContentProvider(scheme, {
        onDidChange: changed.event,
        async provideTextDocumentContent(uri: vscode.Uri, token: vscode.CancellationToken) {
            const opts = extractOptions(uri);
            const fileUri = uriFriendlyBase16Decode(uri.query);
            const outputs = await channel.sendRequest<OutputResult>('textDocument/$/retrieve_outputs', { textDocument: { uri: fileUri } }, token).catch(e => {
                vscode.window.showErrorMessage('Error encountered while fetching output document: ' + e);
                return [] as OutputResult;
            });
            return outputs.filter(x => opts.mnote && x.level >= 0 || opts.punch && x.level < 0).map(outputLineToString).join('\n');
        },
    });
    const notification = channel.onNotification('$/retrieve_outputs', x => {
        if (x && ('uri' in x) && typeof x.uri === 'string') {
            const uri = vscode.Uri.parse(x.uri);
            changed.fire(createOutputUri(uri, { mnote: true, punch: true }));
            changed.fire(createOutputUri(uri, { mnote: false, punch: true }));
            changed.fire(createOutputUri(uri, { mnote: true, punch: false }));
        }
    });

    disposables.push(changed, provider, notification);
}
