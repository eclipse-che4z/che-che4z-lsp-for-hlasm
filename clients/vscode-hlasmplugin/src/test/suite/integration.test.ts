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
import { DidSaveTextDocumentNotification } from 'vscode-languageclient';

suite('Integration Test Suite', () => {
	// extension was activated
	test('Activated test', () => {
		const ext = vscode.extensions.getExtension('BroadcomMFD.hlasm-language-support');
		assert.notEqual(ext, undefined);
		assert.ok(ext.isActive);
	});

	// correct workspace is open
	test('Workspace test', () => {
		const workspaces = vscode.workspace.workspaceFolders;
		assert.ok(workspaces);
		assert.equal(workspaces[0].name, 'workspace');
	});

	// open 'open' file, should be recognized as hlasm with no errors
	test('HLASM file open', async () => {
		// prepare workspace
		await vscode.commands.executeCommand('workbench.action.closeAllEditors');
		assert.equal(vscode.window.activeTextEditor,undefined);
		// 'open' should be in workspace
		const files = await vscode.workspace.findFiles('open');
		assert.ok(files && files[0]);
		const file = files[0];
		// open the file
		const document = await vscode.workspace.openTextDocument(file);
		assert.equal(document.languageId,'hlasm');
		const diagnostics = vscode.languages.getDiagnostics();
		assert.equal(diagnostics.length,0);
	}).slow(2000);
});
