import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient/node';
import assert = require('assert');

class file_content {
    content: string;
}

export class hlasmVirtualFileContentProvider implements vscode.TextDocumentContentProvider {
    onDidChange?: vscode.Event<vscode.Uri> = undefined;
    provideTextDocumentContent(uri: vscode.Uri, token: vscode.CancellationToken): vscode.ProviderResult<string> {
        return new Promise((resolve, reject) => {
            this.client.onReady().then(() => {
                assert(uri.scheme === 'hlasm');
                const trimmed = uri.authority.trim();
                const file_id = +trimmed;
                if (trimmed.length > 0 && !isNaN(file_id))
                    this.client.sendRequest<file_content>("get_file_content", { id: file_id }, token)
                        .then(c => resolve(c.content))
                        .catch(e => reject(e));

                else
                    reject("Invalid virtual HLASM file.");
            });
        });
    }

    constructor(private client: vscodelc.BaseLanguageClient) {
    }
}
