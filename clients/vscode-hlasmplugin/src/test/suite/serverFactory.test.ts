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
    
    test('TCP server options test', (done) => {
        // create TCP server options
        factory.create(true).then((options) => {
            (<(() => Thenable<vscodelc.StreamInfo>)>(options))().then((streamInfoOptions) => {
                console.log('created');
                // retrieve one of the sockets
                var socket = <net.Socket>(streamInfoOptions.writer);
                // when the socket is connected, check its address and port
                socket.on('connect', () => {
                    console.log('connected');
                    assert.equal(socket.remoteAddress, '127.0.0.1');
                    assert.notEqual(socket.remotePort, factory.dapPort);
                    assert.ok(socket.remotePort > 1024 && socket.remotePort < 65535);
                    done();
                });
            });
        });
    }).slow(2000);

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
    });

    test('DAP port test', () => {
        assert.ok(factory.dapPort > 1024 && factory.dapPort < 65535)
    })
});