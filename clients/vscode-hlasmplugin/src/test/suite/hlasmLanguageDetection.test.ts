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
		const existingDocument = await vscode.workspace.openTextDocument(path.join(vscode.workspace.workspaceFolders![0].uri.fsPath, 'open'));
		const document = new TextDocumentMock();
		document.fileName = "file";
		// set plain text file to HLASM
		document.languageId = 'plaintext';
		document.text = hlasmContents;
		document.uri = existingDocument.uri;
		assert.ok(detector.setHlasmLanguage(document));

		// set HLASM file to HLASM
		document.languageId = 'hlasm';
		assert.ok(detector.setHlasmLanguage(document));
	});
	// if file has no extension  return false
	test('non HLASM file extension test false_one', async () => {
		assert.ok(detector.withoutExtension(vscode.Uri.file('file')));
	});
	test('non HLASM file extension test false_two', async () => {
		assert.ok(detector.withoutExtension(vscode.Uri.file('.hidden')));
	});
	test('non HLASM file extension test false_three', async () => {
		assert.ok(detector.withoutExtension(vscode.Uri.file('file.')));
	});
	test('Cobol file containing several dots but not extension returns false', async () => {
		assert.ok(detector.withoutExtension(vscode.Uri.file('USER.FTP.COBOL(BIGFILE)')));
	});
	test('Cobol file ending with extension', async () => {
		assert.ok(!detector.withoutExtension(vscode.Uri.file('USER.FTP.COBOL(BIGFILE).cbl')));
	});
	// if file has extension  return true
	test('non HLASM file extension test true', async () => {
		assert.ok(!detector.withoutExtension(vscode.Uri.file('file.cbl')));

	});
	//Dont set to Hlasm if file extension exist other than .hlasm or file assosciations irrespective of content
	test('non HLASM test based on file extension and language id', async () => {
		const document = new TextDocumentMock();
		// set plain text file to HLASM
		document.languageId = 'plaintext';
		document.text = hlasmContents;
		document.fileName = "file.cbl";
		document.uri = vscode.Uri.file('file.cbl');
		assert.ok(!detector.setHlasmLanguage(document));

	});
	test('HLASM test based on file extension and language id', async () => {
		const document = new TextDocumentMock();
		// set plain text file to HLASM
		document.languageId = 'hlasm';
		document.text = hlasmContents;
		document.fileName = "file.hlasm";
		document.uri = vscode.Uri.file('file.hlasm');
		assert.ok(detector.setHlasmLanguage(document));

	});

	// set non HLASM file as HLASM
	test('non HLASM files test', async () => {
		const document = new TextDocumentMock();
		// set plain text file to HLASM
		document.languageId = 'plaintext';
		document.text = nonHlasmContents;
		document.uri = vscode.Uri.file('file');
		document.fileName = "file";
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
