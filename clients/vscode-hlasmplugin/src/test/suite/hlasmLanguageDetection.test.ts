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
import * as path from 'path';
import * as vscode from 'vscode';

import { HLASMLanguageDetection } from '../../hlasmLanguageDetection';
import { ConfigurationsHandler } from '../../configurationsHandler';
import { TextDocumentMock } from '../mocks';

suite('Language Detection Test Suite', () => {
	const handler = new ConfigurationsHandler();
	const detector = new HLASMLanguageDetection(handler);

	// set HLASM file as HLASM
	test('HLASM files test', async () => {
		const existingDocument = await vscode.workspace.openTextDocument(path.join(vscode.workspace.workspaceFolders[0].uri.fsPath, 'open'));
		const document = new TextDocumentMock();

		// set plain text file to HLASM
		document.languageId = 'plaintext';
		document.text = hlasmContents;
		document.uri = existingDocument.uri;
		assert.ok(detector.setHlasmLanguage(document));

		// set HLASM file to HLASM
		document.languageId = 'hlasm';
		assert.ok(detector.setHlasmLanguage(document));
	});

	// set non HLASM file as HLASM
	test('non HLASM files test', async () => {
		var document = new TextDocumentMock();
		// set plain text file to HLASM
		document.languageId = 'plaintext';
		document.text = nonHlasmContents;
		document.uri = vscode.Uri.file('file');
		assert.ok(!detector.setHlasmLanguage(document));
	});
});

const hlasmContents = `
 LR 1,1 remarks                                                        X
			   that continue here
* comments that do not count
 UNK unknown operand`

const nonHlasmContents = `
a simple readme file
nothing to seee here`