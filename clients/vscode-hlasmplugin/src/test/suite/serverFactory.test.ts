/*
 * Copyright (c) 2019 Broadcom.
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
import * as vscodelc from 'vscode-languageclient';
import * as glob from 'glob';
import * as net from 'net';

import { ServerFactory } from '../../serverFactory'

suite('ServerFactory Test Suite', () => {
    var factory = new ServerFactory();
     
    test('non TCP server options test', async () => {
        // create standard server options
        const options = await factory.create(false);
        // retrieve executable
        const execOptions = <vscodelc.Executable>(options);
        // check command
        glob(execOptions.command + '*', (err, matches) => {
            assert.ok(matches.length > 0);
        });
        // check port arguments
        assert.equal(execOptions.args.length, 2);
        assert.equal(execOptions.args[0], '-p');
        assert.equal(execOptions.args[1], factory.dapPort.toString());
        assert.ok(factory.dapPort > 1024 && factory.dapPort < 65535)
    });
});