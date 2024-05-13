/*
 * Copyright (c) 2024 Broadcom.
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

import { BaseLanguageClient } from 'vscode-languageclient';
import { pickUser } from './uiUtils';
import { isCancellationError } from './helpers';

const logOptions = [
    { label: 'Error', value: 0 },
    { label: 'Warning', value: 1 },
    { label: 'Info', value: 2 },
];

export async function setServerLogLevel(client: BaseLanguageClient) {
    try { client.sendNotification("set_log_level", { 'log-level': await pickUser('Select log level', logOptions) }); }
    catch (e) { if (!isCancellationError(e)) throw e; }
}
