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

import * as vscode from 'vscode';

// debug configuration provider, adds port number dynamically
export class HLASMConfigurationProvider implements vscode.DebugConfigurationProvider {
    resolveDebugConfiguration(folder: vscode.WorkspaceFolder | undefined,
        config: vscode.DebugConfiguration)
        : vscode.ProviderResult<vscode.DebugConfiguration> {
        // no launch.json, debug current
        if (!config.type && !config.request && !config.name) {
            config.type = "hlasm";
            config.request = "launch";
            config.name = "Macro tracer: current program";
            config.program = "${command:extension.hlasm-plugin.getCurrentProgramName}";
            config.stopOnEntry = true;
        }
        return config;
    }
}

// show an input box to select the program to trace
export function getProgramName() {
    return vscode.window.showInputBox({
        placeHolder: "Please enter the name of a program in the workspace folder",
        value: "pgm"
    });
}

// checks whether the currently open file is hlasm and starts tracing if so
export function getCurrentProgramName() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('No file open.');
        return undefined;
    }
    if (editor.document.languageId != 'hlasm') {
        vscode.window.showErrorMessage(editor.document.fileName + ' is not a HLASM file.');
        return undefined;
    }
    return editor.document.uri.scheme === 'file' ? editor.document.fileName : editor.document.uri.toString();
}
