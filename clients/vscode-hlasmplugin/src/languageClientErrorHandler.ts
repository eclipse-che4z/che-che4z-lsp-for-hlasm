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

import * as vscodelc from 'vscode-languageclient';

import { Telemetry } from './telemetry';

export class LanguageClientErrorHandler implements vscodelc.ErrorHandler {
    defaultHandler?: vscodelc.ErrorHandler = undefined;
    telemetry: Telemetry;

    constructor(tlmtry: Telemetry) {
        this.telemetry = tlmtry;
    }

    error(error: Error, message: vscodelc.Message | undefined, count: number | undefined): vscodelc.ErrorHandlerResult | Promise<vscodelc.ErrorHandlerResult> {
        this.telemetry.reportException(error, { message: message?.jsonrpc ?? '', count: '' + count });

        return this.defaultHandler!.error(error, message, count);
    }

    closed(): vscodelc.CloseHandlerResult | Promise<vscodelc.CloseHandlerResult> {
        this.telemetry.reportErrorEvent("hlasm.connectionClosed");
        return this.defaultHandler!.closed();
    }

}
