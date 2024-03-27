/*
 * Copyright (c) 2022 Broadcom.
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

export const pgm_conf_file = 'pgm_conf.json';
export const proc_grps_file = 'proc_grps.json';
export const bridge_json_file = '.bridge.json';
export const hlasmplugin_folder = '.hlasmplugin';
export const ebg_folder = '.ebg';

export const hlasmplugin_folder_filter: vscode.DocumentFilter = { language: 'json', pattern: '**/.hlasmplugin/*' };
export const bridge_json_filter: vscode.DocumentFilter = { language: 'json', pattern: '**/.bridge.json' };

export const EXTENSION_ID = "broadcommfd.hlasm-language-support";

export const continuationColumn = 71;
export const initialBlanks = 15;

export const languageIdHlasm = 'hlasm';
export const languageIdHlasmListing = 'hlasmListing';

export const debugTypeHlasm = 'hlasm';

export const schemeExternalFiles = 'hlasm-external';
export const schemeVirtualFiles = 'hlasm';
export const schemeOutput = 'hlasm-output';
