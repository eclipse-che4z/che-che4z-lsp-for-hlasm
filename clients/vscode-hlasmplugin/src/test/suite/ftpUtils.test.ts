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

import * as assert from 'assert';
import { connectionSecurityLevel, translateConnectionInfo } from "../../mfCreds";

suite('FTP Utilities', () => {

    test('Connection info translation', () => {
        assert.deepStrictEqual(translateConnectionInfo({
            host: 'h',
            port: 12345,
            user: 'u',
            password: 'p',
            securityLevel: connectionSecurityLevel.unsecure
        }), {
            host: 'h',
            user: 'u',
            password: 'p',
            port: 12345,
            secure: false,
            secureOptions: undefined
        });
        assert.deepStrictEqual(translateConnectionInfo({
            host: 'h',
            port: 12345,
            user: 'u',
            password: 'p',
            securityLevel: connectionSecurityLevel.acceptUnauthorized
        }), {
            host: 'h',
            user: 'u',
            password: 'p',
            port: 12345,
            secure: true,
            secureOptions: { rejectUnauthorized: false }
        });
        assert.deepStrictEqual(translateConnectionInfo({
            host: 'h',
            port: 12345,
            user: 'u',
            password: 'p',
            securityLevel: connectionSecurityLevel.rejectUnauthorized
        }), {
            host: 'h',
            user: 'u',
            password: 'p',
            port: 12345,
            secure: true,
            secureOptions: { rejectUnauthorized: true }
        });
    });

});

