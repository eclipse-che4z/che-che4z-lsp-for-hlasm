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
import * as vscodelc from 'vscode-languageclient';

type OpcodeSuggestionList = {
    [key: string]: {
        opcode: string;
        distance: number;
    }[]
};

interface OpcodeSuggestionResponse {
    uri: string;
    suggestions: OpcodeSuggestionList;
};

function unique(a: string[]) {
    return [...new Set(a)];
}

async function gatherOpcodeSuggestions(opcodes: string[], client: vscodelc.BaseLanguageClient, uri: vscode.Uri): Promise<OpcodeSuggestionList> {
    const suggestionsResponse = await client.sendRequest<OpcodeSuggestionResponse>("textDocument/$/opcode_suggestion", {
        textDocument: { uri: uri.toString() },
        opcodes: opcodes,
        extended: false,
    });

    return suggestionsResponse.suggestions;

}

async function opcodeTimeout(timeout: number): Promise<OpcodeSuggestionList> {
    return new Promise<OpcodeSuggestionList>((resolve, _) => { setTimeout(() => { resolve({}); }, timeout); })
}

async function generateCodeActions(opcodeTasks: {
    diag: vscode.Diagnostic;
    opcode: string;
}[], client: vscodelc.BaseLanguageClient, uri: vscode.Uri, timeout: number): Promise<vscode.CodeAction[]> {
    const result: vscode.CodeAction[] = [];
    const suggestions = await Promise.race([gatherOpcodeSuggestions(unique(opcodeTasks.map(x => x.opcode)), client, uri), opcodeTimeout(timeout)]);

    for (const { diag, opcode } of opcodeTasks) {
        if (opcode in suggestions) {
            const subst = suggestions[opcode];
            for (const s of subst) {
                const edit = new vscode.WorkspaceEdit();
                edit.replace(uri, diag.range, s.opcode)
                result.push({
                    title: `Did you mean '${s.opcode}'?`,
                    diagnostics: [diag],
                    kind: vscode.CodeActionKind.QuickFix,
                    edit: edit
                });
            }
        }
    }
    return result;
}

const invalidUTF16Sequences = /[\uD800-\uDBFF](?![\uDC00-\uDFFF])|(?<![\uD800-\uDBFF])[\uDC00-\uDFFF]/g;

export async function generateOpcodeSuggestionsCodeActions(diags: vscode.Diagnostic[], client: vscodelc.BaseLanguageClient, document: vscode.TextDocument): Promise<vscode.CodeAction[]> {
    return await generateCodeActions(diags.map(diag => { return { diag, opcode: document.getText(diag.range).replace(invalidUTF16Sequences, '').toUpperCase() }; }), client, document.uri, 1000);
}
