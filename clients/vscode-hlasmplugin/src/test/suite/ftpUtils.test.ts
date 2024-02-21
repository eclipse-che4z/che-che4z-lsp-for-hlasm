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
import { connectionSecurityLevel, gatherSecurityLevelFromZowe, translateConnectionInfo } from "../../ftpCreds";

suite('FTP Utilities', () => {

    test('zowe profile translation', () => {
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: false }), connectionSecurityLevel.unsecure);

        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({}), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: '' }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: 0 }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({}), connectionSecurityLevel.rejectUnauthorized);

        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true, rejectUnauthorized: true }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true, rejectUnauthorized: '' }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true, rejectUnauthorized: 0 }), connectionSecurityLevel.rejectUnauthorized);
        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true, rejectUnauthorized: '' }), connectionSecurityLevel.rejectUnauthorized);

        assert.strictEqual(gatherSecurityLevelFromZowe({ secureFtp: true, rejectUnauthorized: false }), connectionSecurityLevel.acceptUnauthorized);
    });

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

