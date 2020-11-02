/*
 * Copyright (c) 2019 Broadcom.
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

import * as assert from 'assert';
import * as vscode from 'vscode';

import { SemanticTokensResult } from '../../protocol.semanticTokens';
import { SemanticTokensFeature } from '../../semanticTokens';
import { LanguageClientMock, LanguageClientOptionsMock, TextDocumentMock } from '../mocks';

class SemanticTokensFeatureExtend extends SemanticTokensFeature {
    public setLegend(legend: vscode.SemanticTokensLegend) {
        this.legend = legend;
    }
}

function compareRanges(first: vscode.Range, second: vscode.Range): boolean {
    return first.start.character == second.start.character &&
        first.start.line == second.start.line &&
        first.end.line == second.end.line &&
        first.end.character == second.end.character;
}

suite('HLASM Semantic Tokens Test Suite', () => {

    test('Apply Decorations test', () => {
        // prepare highlighting feature
        const document = new TextDocumentMock();
        document.uri = vscode.Uri.parse('file');
        const options = new LanguageClientOptionsMock();
        const client = new LanguageClientMock('client', 'mock', options);
        const highlight = new SemanticTokensFeatureExtend(client);
        highlight.setLegend(new vscode.SemanticTokensLegend(['label','instruction']))

        // prepare params
        const firstRange = new vscode.Range(new vscode.Position(0, 0), new vscode.Position(0, 2));
        const secondRange = new vscode.Range(new vscode.Position(0, 3), new vscode.Position(0, 6));
        const result: SemanticTokensResult = {
            data: [0,0,2,0,0,0,3,3,1,0]
        };
        client.onReady().then(() => {
            var results = highlight.applyDecorations(document,result.data)
            assert.notEqual(results, undefined);
            const documentMap = results.get(document.uri.toString());
            assert.ok(documentMap);
            assert.ok(compareRanges(documentMap.get('label')[0][0], firstRange));
            assert.ok(compareRanges(documentMap.get('instruction')[0][0], secondRange));
        })
        
    });
})