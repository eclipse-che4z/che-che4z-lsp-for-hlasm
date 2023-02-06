/*
 * Copyright (c) 2021 Broadcom.
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

import * as vscodelc from 'vscode-languageclient/node';

import { ErrorHandler } from 'vscode-languageclient/node';
import { Telemetry } from './telemetry';

export class LanguageClientErrorHandler implements ErrorHandler {
    defaultHandler: ErrorHandler = undefined;
    telemetry: Telemetry;

    constructor(tlmtry: Telemetry) {
        this.telemetry = tlmtry;
    }

    error(error: Error, message: vscodelc.Message, count: number): vscodelc.ErrorAction {
        this.telemetry.reportEvent("hlasm.connectionError", { ...error })

        return this.defaultHandler.error(error, message, count);
    }
    closed(): vscodelc.CloseAction {
        this.telemetry.reportEvent("hlasm.connectionClosed")
        return this.defaultHandler.closed();
    }

}
