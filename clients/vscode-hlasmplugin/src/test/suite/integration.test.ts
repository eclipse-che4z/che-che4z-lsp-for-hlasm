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
			const allDiags = vscode.languages.getDiagnostics();
			listener.dispose();
			var openDiags = allDiags.find(pair => pair[0].path.endsWith("open"))[1]
			
			if (openDiags.length == 1 && openDiags[0].code == 'M003')
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
		openFileEditor.edit(edit => {
			edit.insert(new vscode.Position(7,1),'L');
		}).then(_ => {
			const movePosition = new vscode.Position(7,2);
			openFileEditor.selection = new vscode.Selection(movePosition,movePosition);
			vscode.commands.executeCommand('editor.action.triggerSuggest')
			.then(() => {
				setTimeout(() => {
					vscode.commands.executeCommand('acceptSelectedSuggestion').then(result => {
						setTimeout(() => {
							const text = openFileEditor.document.getText();
							const acceptedLine = text.split('\n')[7];
							if (acceptedLine.includes('L   R,D12U(X,B)'))
								done();
							else
								done('Wrong suggestion result' + acceptedLine)
						}, 1000)
					})
				},1000);
			});
		})
	}).timeout(10000).slow(4000);

	// test completion for variable symbols
	test('Completion Variable symbol test', (done) => {
		assert.equal(vscode.window.activeTextEditor, openFileEditor);
		// add '&' to simulate start of a variable symbol
		openFileEditor.edit(edit => {
			edit.insert(new vscode.Position(8,0),'&');
		}).then(_ => {
			const movePosition = new vscode.Position(8,1);
			openFileEditor.selection = new vscode.Selection(movePosition,movePosition);
			vscode.commands.executeCommand('editor.action.triggerSuggest')
			.then(() => {
				setTimeout(() => {
					vscode.commands.executeCommand('acceptSelectedSuggestion').then(result => {
						setTimeout(() => {
							const text = openFileEditor.document.getText();
							const acceptedLine = text.split('\n')[8];
							if (acceptedLine.includes('&VAR'))
								done();
							else
								done('Wrong suggestion result' + acceptedLine)
						}, 1000)
					})
				},1000);
			});
		})
	}).timeout(10000).slow(4000);

	// go to definition for ordinary symbol
	test('Definition Ordinary symbol test', (done) => {
		assert.equal(vscode.window.activeTextEditor, openFileEditor);
		vscode.commands.executeCommand('vscode.executeDefinitionProvider',openFileEditor.document.uri,new vscode.Position(1,7))
			.then((result: vscode.Location[]) => {
				if (result.length == 1 
					&& result[0].uri.fsPath == openFileEditor.document.fileName
					&& result[0].range.start.line == 9
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

	// debug open code test
	test('Debug test', (done) => {
		// simulates basic debugging procedure
		assert.equal(vscode.window.activeTextEditor, openFileEditor);

		// when the debug session starts
		const disposable = vscode.debug.onDidStartDebugSession(session => {
			// step over once
			// wait a second to let the debug session complete
			setTimeout(() => {
				vscode.commands.executeCommand('workbench.action.debug.stepOver') 
				// wait 1 more second to let step over take place
				// then check for VAR2 variable
				setTimeout(() => {
					session.customRequest('scopes',{frameId:0}).then(scopesResult => {
					const noBody = scopesResult.body == undefined;
					var scopes;
					if (noBody)
						scopes = scopesResult.scopes;
					else
						scopes = scopesResult.body.scopes;
					const reference = scopes.find((scope : {name:string}) => scope.name == 'Locals').variablesReference;
					session.customRequest('variables',{variablesReference: reference}).then(variablesResult => {
						disposable.dispose();
						vscode.commands.executeCommand('workbench.action.debug.stop');
						var variables;
						if (noBody)
							variables = variablesResult.variables;
						else
							variables = variablesResult.body.variables;
						if (variables.length == 1 && variables[0].value == 'SOMETHING' && variables[0].name == '&VAR2')
							done();
						else
							done('Wrong debug variable &VAR2');
					});
				})}, 1000);
			}, 1000)
		});
		// start debugging
		vscode.debug.startDebugging(vscode.workspace.workspaceFolders[0],'Macro tracer: current program');
	}).timeout(10000).slow(4000);
});
