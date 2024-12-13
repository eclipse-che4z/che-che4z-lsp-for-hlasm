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

type Strings = {
    readonly [key: string]: string | undefined;
}

type Numbers = {
    readonly [key: string]: number | undefined;
}

export interface Telemetry {
    reportEvent: (eventName: string, properties?: Strings, measurements?: Numbers) => void,
    reportErrorEvent: (eventName: string, properties?: Strings, measurements?: Numbers) => void,
    reportException: (error: Error, properties?: Strings, measurements?: Numbers) => void,
    dispose: () => void,
}

export function createDummyTelemetry(): Telemetry {
    return {
        reportEvent: () => { },
        reportErrorEvent: () => { },
        reportException: () => { },
        dispose: () => { }
    };
}
