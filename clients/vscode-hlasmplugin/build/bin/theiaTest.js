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

const process = require('process');
const { spawn } = require('child_process');
const puppeteer = require('puppeteer');

const theiaDir = process.argv[2];

async function main() {
    // prepare plugin for theia
    process.env.THEIA_DEFAULT_PLUGINS = 'local-dir:./plugin/';

    // run integration tests as plugin for theia
    const child = spawn('node', [
        theiaDir + '/src-gen/backend/main.js',
        './dist_test/workspace/',
        '--extensionTestsPath=' + process.cwd() + '/dist_test/test/suite',
        '--hostname', '0.0.0.0',
        '--port', '3000'
    ]);

    // give theia 5 seconds to start, then connect
    setTimeout(async function() {
        const browser = await puppeteer.launch({ executablePath: '/usr/lib/chromium/chromium', headless: true, args: ['--no-sandbox', '--disable-gpu'] });
        const page = await browser.newPage();
        try {
            await page.goto('http://localhost:3000');
        }
        catch (err) {
            console.log(err);
        }
    }, 5000);

    // check for the results
    child.stdout.on('data', function(data) {
        console.log(data.toString());
        if (data.toString().includes('>>>THEIA TESTS PASSED<<<')) {
            process.exit();
        }
    })

    child.stderr.on('data', function(data) {
        console.log(data.toString());
        if (data.toString().includes('>>>THEIA TESTS FAILED<<<')) {
            process.exit(2);
        }
    })

    child.on('close', function() {
        process.exit(2);
    })

    // Give all the tests 60 seconds to finish, otherwise timeout
    setTimeout(async function() {
        process.exit(2);
    }, 60000)

}

main();
