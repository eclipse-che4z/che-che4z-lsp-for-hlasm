import * as assert from 'assert';
import * as vscode from 'vscode';

suite('Extension Test Suite', () => {
	// extension was activated
	test('Activated test', () => {
		const extension = vscode.extensions.getExtension('BroadcomMFD.hlasm-language-support');
		assert.notEqual(extension,undefined);
		assert.ok(extension.isActive);
	});
	
	// continuation commands are registered
	test('Commands registered test', async () => {
		var commandList = await vscode.commands.getCommands();
		assert.notEqual(commandList.find(command => command == 'insertContinuation'), undefined);
		assert.notEqual(commandList.find(command => command == 'removeContinuation'), undefined);
	});
});