/*
 * Copyright (c) 2020 Broadcom.
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
import * as vscode from "vscode";

import TelemetryReporter from 'vscode-extension-telemetry';

const EXTENSION_ID = "broadcommfd.hlasm-language-support";
const TELEMETRY_DEFAULT_KEY = "NOT_TELEMETRY_KEY";

// The following line is replaced by base64 encoded telemetry key in the CI
const TELEMETRY_KEY_ENCODED = TELEMETRY_DEFAULT_KEY;

export class Telemetry {

    private reporter: TelemetryReporter;
    private telemetry_key: string = undefined;

    private getExtensionVersion(): string {
        return vscode.extensions.getExtension(EXTENSION_ID).packageJSON.version;
    }

    private getTelemetryKey(): string {
        if (this.telemetry_key === undefined)
            this.telemetry_key = Buffer.from(TELEMETRY_KEY_ENCODED, "base64").toString().trim();
        return this.telemetry_key;
    }

    constructor() {
        this.reporter = new TelemetryReporter(EXTENSION_ID, this.getExtensionVersion(), this.getTelemetryKey());
    }

    public reportEvent(eventName: string, properties? : any, measurements? : any): void {
        if (this.isValidTelemetryKey()) {
            this.reporter.sendTelemetryEvent(eventName, properties, measurements);
        }
    }

    public dispose(): any {
        this.reporter.dispose();
    }

    private isValidTelemetryKey(): boolean {
        return this.getTelemetryKey() !== TELEMETRY_DEFAULT_KEY;
    }
}
