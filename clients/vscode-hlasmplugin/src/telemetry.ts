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

import TelemetryReporter, { TelemetryEventMeasurements, TelemetryEventProperties } from '@vscode/extension-telemetry';

const TELEMETRY_DEFAULT_KEY = "NOT_TELEMETRY_KEY";

// The following line is replaced by base64 encoded telemetry key in the CI
const TELEMETRY_KEY_ENCODED = TELEMETRY_DEFAULT_KEY;

export class Telemetry {

    private reporter: TelemetryReporter | null = null;
    private telemetry_key?: string = undefined;

    private getTelemetryKey(): string {
        if (this.telemetry_key === undefined)
            this.telemetry_key = Buffer.from(TELEMETRY_KEY_ENCODED, "base64").toString().trim();
        return this.telemetry_key;
    }

    constructor() {
        try {
            // This is mainly to handle Theia's lack of support
            this.reporter = new TelemetryReporter(this.getTelemetryKey());
        } catch (e) {
            console.log('Error encountered while creating TelemetryReporter:', e);
        }
    }

    public reportEvent(eventName: string, properties?: TelemetryEventProperties, measurements?: TelemetryEventMeasurements): void {
        if (this.isValidTelemetryKey()) {
            this.reporter?.sendTelemetryEvent(eventName, properties, measurements);
        }
    }

    public dispose(): any {
        this.reporter?.dispose();
    }

    private isValidTelemetryKey(): boolean {
        return this.getTelemetryKey() !== TELEMETRY_DEFAULT_KEY;
    }
}
