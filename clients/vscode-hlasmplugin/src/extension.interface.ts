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

import * as vscode from 'vscode'
import { ConfigurationProviderRegistration, HLASMExternalConfigurationProviderHandler } from './hlasmExternalConfigurationProvider';
import { ClientInterface, ClientUriDetails, ExternalFilesInvalidationdata, ExternalRequestType } from './hlasmExternalFiles';

export interface HlasmExtension {
    registerExternalFileClient<ConnectArgs, ReadArgs extends ClientUriDetails, ListArgs extends ClientUriDetails>(service: string, client: Readonly<ClientInterface<ConnectArgs, ReadArgs, ListArgs>>): vscode.Disposable;
    registerExternalConfigurationProvider(h: HLASMExternalConfigurationProviderHandler): ConfigurationProviderRegistration;
};

export { ExternalRequestType, ExternalFilesInvalidationdata };
