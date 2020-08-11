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

import { SemanticHighlightingParams } from '../../semanticHighlighting';
import { ContinuationInfo, HLASMSemanticHighlightingFeature, HLASMSemanticHighlightingParams } from '../../hlasmSemanticHighlighting';
import { LanguageClientMock, LanguageClientOptionsMock } from '../mocks';

suite('HLASM Semantic Highlighting Test Suite', () => {

    test('Only Highlighting Apply Decorations test', () => {
        // prepare highlighting feature
        const options = new LanguageClientOptionsMock();
        const client = new LanguageClientMock('client', 'mock', options);
        const highlight = new HLASMSemanticHighlightingFeature(client);

        // prepare params
        const firstRange = new vscode.Range(new vscode.Position(0, 0), new vscode.Position(0, 2));
        const secondRange = new vscode.Range(new vscode.Position(1, 0), new vscode.Position(1, 2));
        const params: SemanticHighlightingParams = {
            textDocument: {
                version: 0,
                uri: 'file'
            },
            tokens: [{
                scope: 'instruction',
                lineStart: firstRange.start.line,
                lineEnd: firstRange.end.line,
                columnStart: firstRange.start.character,
                columnEnd: firstRange.end.character
            }, {
                scope: 'label',
                lineStart: secondRange.start.line,
                lineEnd: secondRange.end.line,
                columnStart: secondRange.start.character,
                columnEnd: secondRange.end.character
            }]
        }
        const results = highlight.applyDecorations(params);
        const documentMap = results.get(vscode.Uri.file('file').toString())
        assert.ok(documentMap);
        assert.ok(compareRanges(documentMap.get('instruction')[0][0], firstRange));
        assert.ok(compareRanges(documentMap.get('label')[0][0], secondRange));
    });

    test('Only Continuations Apply Decorations test', () => {
        // prepare highlighting feature
        const options = new LanguageClientOptionsMock();
        const client = new LanguageClientMock('client', 'mock', options);
        const highlight = new HLASMSemanticHighlightingFeature(client);
        // prepare decoration params
        const contInfo = new ContinuationInfo();
        contInfo.continuationPositions = [{ line: 0, continuationPosition: 10 }, { line: 1, continuationPosition: 11 }];
        contInfo.global = {
            continuationColumn: 11,
            continueColumn: 2
        }
        const params: HLASMSemanticHighlightingParams = {
            continuation: contInfo,
            textDocument: {
                version: 0,
                uri: 'file'
            },
            tokens: []
        }

        highlight.applyDecorations(params);
        // get continuation info
        const info = highlight.getContinuationInfo().get('file');
        assert.ok(info);
        assert.equal(info.continuationColumn, 11);
        assert.equal(info.continueColumn, 2);
        assert.equal(info.lineContinuations.get(0), 10);
        assert.equal(info.lineContinuations.get(1), 11);
    });
});

function compareRanges(first: vscode.Range, second: vscode.Range): boolean {
    return first.start.character == second.start.character &&
        first.start.line == second.start.line &&
        first.end.line == second.end.line &&
        first.end.character == second.end.character;
}
