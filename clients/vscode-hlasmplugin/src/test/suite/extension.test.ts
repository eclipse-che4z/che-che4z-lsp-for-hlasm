import * as assert from 'assert';
import * as vscode from 'vscode';

suite('Extension Test Suite', () => {
	// extension was activated
	test('Activated test', () => {
		const ext = vscode.extensions.getExtension('BroadcomMFD.hlasm-language-support');
		assert.notEqual(ext, undefined);
		assert.ok(ext.isActive);
	});
});