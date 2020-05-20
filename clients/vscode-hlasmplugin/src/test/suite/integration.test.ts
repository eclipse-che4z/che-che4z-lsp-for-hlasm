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
import * as path from 'path';

suite('Integration Test Suite', () => {
	var openFileEditor: vscode.TextEditor = null;
	const workspacePath = vscode.workspace.workspaceFolders[0].uri.fsPath;

	// open 'open' file, should be recognized as hlasm
	test('HLASM file open test', (done) => {
		// 'open' should be in workspace
		vscode.workspace.findFiles('open').then(files => {
			assert.ok(files && files[0]);
			const file = files[0];
			// open the file
			vscode.workspace.openTextDocument(file).then(document => {
				vscode.window.showTextDocument(document).then(editor => {
					openFileEditor = editor;
					// setting a language takes a while but shouldn't take longer than a second
					setTimeout(function () {
						if (document.languageId != 'hlasm')
							done('Wrong language');
						else
							done();
					}, 1000);

				});
			});
		});
	}).timeout(10000).slow(4000);

	// change 'open' file to create diagnostic
	test('Diagnostic test', (done) => {
		assert.equal(vscode.window.activeTextEditor, openFileEditor);
		// register callback to check for the correctness of the diagnostic
		const listener = vscode.languages.onDidChangeDiagnostics(_ => {
			const diags = vscode.languages.getDiagnostics();
			listener.dispose();
			if (diags.length, 1 && diags[0][1][0].code == 'M003')
				done();
			else
				done('Wrong diagnostic'); 
		})
		// remove second parameter from LR instruction
		openFileEditor.edit(edit => {
			edit.delete(new vscode.Range(new vscode.Position(2,6), new vscode.Position(2,7)));
		});
	}).timeout(10000).slow(1000);

	// test completion for instructions
	test('Completion Instructions test', (done) => {
		assert.equal(vscode.window.activeTextEditor, openFileEditor);

		vscode.commands.executeCommand('vscode.executeCompletionItemProvider',openFileEditor.document.uri,new vscode.Position(7,1))
			.then((result: vscode.CompletionList) => {
				if (result.items.length == 2046)
					done();
				else
					done('Incorrect number of suggested items ' + result.items.length);
			});
	}).timeout(10000).slow(1000);

	// test completion for variable symbols
	test('Completion Variable symbol test', (done) => {
		assert.equal(vscode.window.activeTextEditor, openFileEditor);
		// add '&' to simulate start of a variable symbol
		openFileEditor.edit(edit => {
			edit.insert(new vscode.Position(7,0),'&');
		}).then(_ => {
			vscode.commands.executeCommand('vscode.executeCompletionItemProvider',openFileEditor.document.uri,new vscode.Position(7,1))
			.then((result: vscode.CompletionList) => {
				if (result.items.length == 2)
					done();
				else
					done('Incorrect number of suggested items ' + result.items.length);
			});
		})
	}).timeout(10000).slow(1000);

	// go to definition for ordinary symbol
	test('Definition Ordinary symbol test', (done) => {
		assert.equal(vscode.window.activeTextEditor, openFileEditor);
		vscode.commands.executeCommand('vscode.executeDefinitionProvider',openFileEditor.document.uri,new vscode.Position(1,7))
			.then((result: vscode.Location[]) => {
				if (result.length == 1 
					&& result[0].uri.fsPath == openFileEditor.document.fileName
					&& result[0].range.start.line == 8
					&& result[0].range.start.character == 0)
					done();
				else
					done('Wrong ordinary symbol definition location');
			});
	}).timeout(10000).slow(1000);

	// hover for variable symbol
	test('Hover Variable symbol test', (done) => {
		assert.equal(vscode.window.activeTextEditor, openFileEditor);
		vscode.commands.executeCommand('vscode.executeHoverProvider',openFileEditor.document.uri,new vscode.Position(6,8))
			.then((result: vscode.Hover[]) => {
				if (result.length == 1
					&& result[0].contents.length == 1
					&& (result[0].contents[0] as vscode.MarkdownString).value == 'number')
					done();
				else
					done('Wrong variable symbol hover contents');
			});
	}).timeout(10000).slow(1000);

	// debug open code test
	test('Debug test', (done) => {
		// simulates basic debugging procedure
		assert.equal(vscode.window.activeTextEditor, openFileEditor);

		// when the debug session starts
		vscode.debug.onDidStartDebugSession(() => {
			// step over once
			// wait a second to let the debug session complete
			setTimeout(() => {
				vscode.commands.executeCommand('workbench.action.debug.stepOver') 
				// wait 1 more second to let step over take place
				// then check for VAR2 variable
				setTimeout(() => {
					vscode.debug.activeDebugSession.customRequest('scopes',{frameId:0}).then((scopesResult: {scopes: {name: string, variablesReference: number}[]}) => {
					const reference = scopesResult.scopes.find(scope => scope.name == 'Locals').variablesReference;
					vscode.debug.activeDebugSession.customRequest('variables',{variablesReference: reference}).then(variablesResult => {
						if (variablesResult.variables.length == 1 && variablesResult.variables[0].value == 'SOMETHING' && variablesResult.variables[0].name == 'VAR2')
							done();
						else
							done('Wrong debug variable VAR2');
					});
				})}, 1000);
			}, 1000)
		});
		// start debugging
		vscode.debug.startDebugging(vscode.workspace.workspaceFolders[0],'Macro tracer: current program');
	}).timeout(10000).slow(3000);

	// go to definition for macros
	test('Definition Macro test', (done) => {
		assert.equal(vscode.window.activeTextEditor, openFileEditor);
		vscode.commands.executeCommand('vscode.executeDefinitionProvider',openFileEditor.document.uri,new vscode.Position(6,2))
			.then((result: vscode.Location[]) => {
				if (result.length == 1 
					&& result[0].uri.fsPath == path.join(workspacePath, 'libs','mac.asm')
					&& result[0].range.start.line == 1
					&& result[0].range.start.character == 4)
					done();
				else
					done('Wrong macro definition location');
			});
	}).timeout(10000).slow(1000);	
});
