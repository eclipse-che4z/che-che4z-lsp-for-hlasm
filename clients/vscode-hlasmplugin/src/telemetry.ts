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
import { decodeBase64 } from './tools';

const TELEMETRY_DEFAULT_KEY: string = "NOT_TELEMETRY_KEY";

// The following line is replaced by base64 encoded telemetry key in the CI
const TELEMETRY_KEY_ENCODED = TELEMETRY_DEFAULT_KEY;

export interface Telemetry {
    reportEvent: (eventName: string, properties?: TelemetryEventProperties, measurements?: TelemetryEventMeasurements) => void,
    reportErrorEvent: (eventName: string, properties?: TelemetryEventProperties, measurements?: TelemetryEventMeasurements) => void,
    reportException: (error: Error, properties?: TelemetryEventProperties, measurements?: TelemetryEventMeasurements) => void,
    dispose: () => void,
}

export function createTelemetry(): Telemetry {
    try { // This is mainly to handle Theia's lack of support
        if (TELEMETRY_KEY_ENCODED !== TELEMETRY_DEFAULT_KEY) {
            const reporter = new TelemetryReporter(decodeBase64(TELEMETRY_KEY_ENCODED).trim());
            return {
                reportEvent: reporter.sendTelemetryEvent.bind(reporter),
                reportErrorEvent: reporter.sendTelemetryErrorEvent.bind(reporter),
                reportException: reporter.sendTelemetryException.bind(reporter),
                dispose: reporter.dispose.bind(reporter),
            };
        }
    }
    catch (e) {
        console.log('Error encountered while creating TelemetryReporter:', e);
    }

    return {
        reportEvent: () => { },
        reportErrorEvent: () => { },
        reportException: () => { },
        dispose: () => { }
    };
}

