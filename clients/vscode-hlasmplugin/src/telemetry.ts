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
import * as fs from "fs";
import * as path from "path";
import * as vscode from "vscode";

import TelemetryReporter from 'vscode-extension-telemetry';
import TelemetryEventMeasurements from 'vscode-extension-telemetry';

const TELEMETRY_KEY = "9cdad008-5b0d-442e-a8e6-55788e858fc0";
const EXTENSION_ID = "broadcommfd.hlasm-language-support"

export class Telemetry {

    private getExtensionVersion(): string {
        return vscode.extensions.getExtension(EXTENSION_ID).packageJSON.version;
    }

    /**
     * This method return the value of the instrumentation key necessary to create the telemetry reporter from an
     * external file configuration. If the file doesn't exists it returns a generic value that will not be valid
     * for collect telemetry event.
     
    private static getTelemetryKeyId(): string {
        return fs.existsSync(this.getTelemetryResourcePath()) ? this.getInstrumentationKey() : TELEMETRY_DEFAULT_CONTENT;
    }

    private static getTelemetryResourcePath() {
        return vscode.Uri.file(
            path.join(ExtensionUtils.getExtensionPath(), "resources", "TELEMETRY_KEY")).fsPath;
    }

    private static getInstrumentationKey(): string {
        return Buffer.from(fs.readFileSync(this.getTelemetryResourcePath(), "utf8"), "base64").toString().trim();
    }

    private static convertData(content: TelemetryEvent) {
        return {
            categories: content.categories.toString(),
            event: content.eventName,
            IDE: ExtensionUtils.getIDEName(),
            notes: content.notes,
            timestamp: content.timestamp,
            rootCause: content.rootCause,
        };
    }


    private static convertMeasurements(content: Map<string, number>): TelemetryMeasurement {
        const result: TelemetryMeasurement = {};

        if (content) {
            for (const [key, value] of content) {
                if (value) {
                    result[key] = value;
                }
            }
        }
        return result;
    }
*/
    private reporter: TelemetryReporter;

    constructor() {
        this.reporter = new TelemetryReporter(EXTENSION_ID, this.getExtensionVersion(), TELEMETRY_KEY);
    }

    public reportEvent(eventName: string, content : any): void {
        if (this.isValidTelemetryKey()) {
            
            this.reporter.sendTelemetryEvent(eventName, content, { 'numericMeasure': 123 });
        }
    }

    /*public reportExceptionEvent(content: any): void {
        if (this.isValidTelemetryKey()) {
            this.reporter.sendTelemetryErrorEvent(content.eventName, TelemetryReporterImpl.convertData(content));
        }
    }*/

    public dispose(): any {
        this.reporter.dispose();
    }

    private isValidTelemetryKey(): boolean {
        return true;
    }
}