/*
 * Copyright (c) 2023 Broadcom.
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

export function askUser(prompt: string, password: boolean, defaultValue: string = ''): Promise<string> {
    const input = vscode.window.createInputBox();
    return new Promise<string>((resolve, reject) => {
        input.ignoreFocusOut = true;
        input.prompt = prompt;
        input.password = password;
        input.value = defaultValue || '';
        input.onDidHide(() => reject(new vscode.CancellationError()));
        input.onDidAccept(() => resolve(input.value));
        input.show();
    }).finally(() => { input.dispose(); });
}

export function pickUser<T>(title: string, options: readonly { label: string, value: T }[]): Promise<T> {
    const input = vscode.window.createQuickPick();
    return new Promise<T>((resolve, reject) => {
        input.ignoreFocusOut = true;
        input.title = title;
        input.items = options.map(x => { return { label: x.label }; });
        input.canSelectMany = false;
        input.onDidHide(() => reject(new vscode.CancellationError()));
        input.onDidAccept(() => resolve(options.find(x => x.label === input.selectedItems[0].label)!.value));
        input.show();
    }).finally(() => { input.dispose(); });
}
