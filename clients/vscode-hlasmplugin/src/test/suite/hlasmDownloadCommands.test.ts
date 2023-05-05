/*
 * Copyright (c) 2022 Broadcom.
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
import { PassThrough, Writable } from 'stream';
import { Uri } from 'vscode';

import { downloadDependenciesWithClient, extractDsn, gatherDownloadList, JobDescription, replaceVariables, adjustJobHeader } from '../../hlasmDownloadCommands';
import { isCancellationError } from '../../helpers';

suite('HLASM Download data sets', () => {
    const getClient = (listResponses: JobDescription[][]) => {
        return {
            setListMaskCalls: new Array<string>(),
            listCalls: 0,
            jcls: new Array<string>(),
            downloadRequests: new Array<{ id: string, spoolFile: number }>(),
            disposeCalls: 0,
            nextJobId: 0,

            dispose() { ++this.disposeCalls },
            async download(target: Writable, id: string, spoolFile: number) {
                if (target instanceof Writable) {
                    this.downloadRequests.push({ id, spoolFile });
                }
                else
                    assert.fail("Writable stream expected");
            },
            async list() {
                const toReturn = this.listCalls++;
                if (toReturn >= listResponses.length)
                    return [];
                else
                    return listResponses[toReturn];
            },
            async setListMask(mask: string) {
                this.setListMaskCalls.push(mask);
            },
            async submitJcl(jcl: string) {
                this.jcls.push(jcl);
                return "JOBID" + this.nextJobId++;
            },
        }
    }

    const getIoOps = () => {
        return {
            unterseCalls: new Array<string>(),
            translateCalls: new Array<string>(),
            copyCalls: new Array<{ source: string, target: string }>(),
            async unterse(outDir: string) {
                this.unterseCalls.push(outDir);

                const process = Promise.resolve();
                const input = new PassThrough();
                return { process, input };
            },
            async translateFiles(dir: string) {
                this.translateCalls.push(dir);
            },
            async copyDirectory(source: string, target: string) {
                this.copyCalls.push({ source, target });
            },
        };
    }

    const getStageCounter = () => {
        return {
            stages: 0,

            stageCompleted() {
                ++this.stages;
            },
        };
    }

    test('Simple jobcard', async () => {
        const client = getClient([
            [],
            [{ jobname: "JOBNAME", id: "JOBID0", details: "RC=0000 3 spool files" }]
        ]);
        const io = getIoOps();
        const stages = getStageCounter();

        assert.deepEqual(await downloadDependenciesWithClient(
            client,
            [{ dsn: 'A.B', dirs: ['/dir1'] }],
            '//JOBNAME JOB 1',
            stages,
            io,
            () => false),
            { failed: [], total: 1 }
        );

        assert.equal(client.disposeCalls, 1);
        assert.deepEqual(client.downloadRequests, [{ id: "JOBID0", spoolFile: 3 }]);
        assert.equal(client.jcls.length, 1);
        assert.ok(client.jcls[0].startsWith("//JOBNAME JOB 1"));
        assert.notEqual(client.jcls[0].indexOf("DSN=A.B"), -1);
        assert.equal(client.listCalls, 2);
        assert.deepEqual(client.setListMaskCalls, ['JOBNAME']);
        assert.equal(io.copyCalls.length, 0);
        assert.deepEqual(io.translateCalls, ['/dir1']);
        assert.deepEqual(io.unterseCalls, ['/dir1']);
        assert.equal(stages.stages, 4);
    });

    test('Jobcard pattern', async () => {
        const client = getClient([
            [{ jobname: "JOBNAME0", id: "JOBID0", details: "RC=0000 3 spool files" }]
        ]);
        const io = getIoOps();
        const stages = getStageCounter();

        assert.deepEqual(await downloadDependenciesWithClient(
            client,
            [{ dsn: 'A.B', dirs: ['/dir1'] }],
            '//JOBNAME? JOB 1',
            stages,
            io,
            () => false),
            { failed: [], total: 1 }
        );

        assert.equal(client.disposeCalls, 1);
        assert.deepEqual(client.downloadRequests, [{ id: "JOBID0", spoolFile: 3 }]);
        assert.equal(client.jcls.length, 1);
        assert.ok(client.jcls[0].startsWith("//JOBNAME0 JOB 1"));
        assert.notEqual(client.jcls[0].indexOf("DSN=A.B"), -1);
        assert.equal(client.listCalls, 1);
        assert.deepEqual(client.setListMaskCalls, ['JOBNAME*']);
        assert.equal(io.copyCalls.length, 0);
        assert.deepEqual(io.translateCalls, ['/dir1']);
        assert.deepEqual(io.unterseCalls, ['/dir1']);
        assert.equal(stages.stages, 4);
    });

    test('Cancelled', async () => {
        const client = getClient([
            [{ jobname: "JOBNAME0", id: "JOBID0", details: "RC=0000 3 spool files" }]
        ]);
        const io = getIoOps();
        const stages = getStageCounter();

        try {
            await downloadDependenciesWithClient(
                client,
                [{ dsn: 'A.B', dirs: ['/dir1'] }],
                '//JOBNAME? JOB 1',
                stages,
                io,
                () => true);
            assert.fail();
        }
        catch (e) {
            assert.ok(isCancellationError(e));
        }

        assert.equal(client.disposeCalls, 1);
        assert.deepEqual(client.downloadRequests, []);
        assert.equal(client.jcls.length, 0);
        assert.equal(client.listCalls, 0);
        assert.deepEqual(client.setListMaskCalls, []);
        assert.equal(io.copyCalls.length, 0);
        assert.deepEqual(io.translateCalls, []);
        assert.deepEqual(io.unterseCalls, []);
        assert.equal(stages.stages, 0);
    });


    test('Multiple data sets', async () => {
        const client = getClient([
            [{ jobname: "JOBNAME0", id: "JOBID0", details: "RC=0000 3 spool files" }],
            [
                { jobname: "JOBNAME0", id: "JOBID0", details: "RC=0000 3 spool files" },
                { jobname: "JOBNAME1", id: "JOBID1", details: "RC=0000 6 spool files" }
            ]
        ]);
        const io = getIoOps();
        const stages = getStageCounter();

        assert.deepEqual(await downloadDependenciesWithClient(
            client,
            [
                { dsn: 'A.B', dirs: ['/dir1'] },
                { dsn: 'C.D', dirs: ['/dir2', '/dir3'] },
            ],
            '//JOBNAME? JOB 1',
            stages,
            io,
            () => false),
            { failed: [], total: 2 }
        );

        assert.equal(client.disposeCalls, 1);
        assert.deepEqual(client.downloadRequests, [{ id: "JOBID0", spoolFile: 3 }, { id: "JOBID1", spoolFile: 6 }]);
        assert.equal(client.jcls.length, 2);
        assert.ok(client.jcls[0].startsWith("//JOBNAME0 JOB 1"));
        assert.notEqual(client.jcls[0].indexOf("DSN=A.B"), -1);
        assert.ok(client.jcls[1].startsWith("//JOBNAME1 JOB 1"));
        assert.notEqual(client.jcls[1].indexOf("DSN=C.D"), -1);
        assert.equal(client.listCalls, 2);
        assert.deepEqual(client.setListMaskCalls, ['JOBNAME*']);
        assert.deepEqual(io.copyCalls, [{ source: '/dir2', target: '/dir3' }]);
        assert.deepEqual(io.translateCalls, ['/dir1', '/dir2']);
        assert.deepEqual(io.unterseCalls, ['/dir1', '/dir2']);
        assert.equal(stages.stages, 4 + 5);
    });

    test('Failed job', async () => {
        const client = getClient([
            [{ jobname: "JOBNAME", id: "JOBID0", details: "RC=0008 3 spool files" }]
        ]);
        const io = getIoOps();
        const stages = getStageCounter();

        assert.deepEqual(await downloadDependenciesWithClient(
            client,
            [{ dsn: 'A.B', dirs: ['/dir1'] }],
            '//JOBNAME JOB 1',
            stages,
            io,
            () => false),
            { failed: [{ dsn: 'A.B', dirs: ['/dir1'] }], total: 1 }
        );

        assert.equal(client.disposeCalls, 1);
        assert.deepEqual(client.downloadRequests, []);
        assert.equal(client.jcls.length, 1);
        assert.ok(client.jcls[0].startsWith("//JOBNAME JOB 1"));
        assert.notEqual(client.jcls[0].indexOf("DSN=A.B"), -1);
        assert.equal(client.listCalls, 1);
        assert.deepEqual(client.setListMaskCalls, ['JOBNAME']);
        assert.equal(io.copyCalls.length, 0);
        assert.equal(stages.stages, 1);
    });

    test('Data set name extractor', () => {
        const ws = Uri.parse("file:///workspace");
        assert.strictEqual(extractDsn("~/dir/MY.DATA.SET", ws)?.dsn, "MY.DATA.SET");
        assert.strictEqual(extractDsn("~/dir/MY.DATA.SET/", ws)?.dsn, "MY.DATA.SET");
        assert.strictEqual(extractDsn("~/dir/MY.DATA.SET//", ws)?.dsn, "MY.DATA.SET");

        if (process.platform === "win32")
            assert.deepEqual(extractDsn("C:\\dir\\my.data.set\\\\", ws), { dsn: "MY.DATA.SET", path: "c:/dir/my.data.set" });
        assert.deepEqual(extractDsn("/home/dir/my.data.set///", ws), { dsn: "MY.DATA.SET", path: "/home/dir/my.data.set" });

        assert.deepEqual(extractDsn("dir\\my.data.set\\\\", ws), { dsn: "MY.DATA.SET", path: "/workspace/dir/my.data.set" });
    });

    test('Config to task conversion', () => {
        const ws = Uri.parse("file:///workspace");

        assert.deepEqual(gatherDownloadList([{ workspaceUri: ws, config: { pgroups: [{ libs: ["ext/MY.DATASET", { path: "ext2/MY.DATASET2" }] }, { libs: ["ext2/MY.DATASET", { path: "ext/OTHER.DATASET" }] }] } }]),
            [
                {
                    dsn: "MY.DATASET",
                    dirs: ["/workspace/ext/MY.DATASET", "/workspace/ext2/MY.DATASET"]
                },
                {
                    dsn: "MY.DATASET2",
                    dirs: ["/workspace/ext2/MY.DATASET2"]
                },
                {
                    dsn: "OTHER.DATASET",
                    dirs: ["/workspace/ext/OTHER.DATASET"]
                }
            ]
        )
    });

    test('Variable replacer', () => {
        const ws = Uri.parse("file:///workspace");
        assert.deepEqual(replaceVariables([{ key: "${workspaceFolder}/${config:test}" }], k => k === 'test' ? 'replacement' : undefined, ws), [{ key: "file:///workspace/replacement" }]);
    });

    test('Job card splitter', () => {
        assert.deepEqual(adjustJobHeader('//ABC JOB'), ['//ABC JOB']);
        assert.deepEqual(adjustJobHeader('//ABC JOB '), ['//ABC JOB']);
        assert.deepEqual(adjustJobHeader('//ABC JOB 123456'), ['//ABC JOB 123456']);
        assert.deepEqual(adjustJobHeader('//ABC JOB 123456 '), ['//ABC JOB 123456']);

        assert.deepEqual(adjustJobHeader('//ABC JOB' + ' '.repeat(100)), ['//ABC JOB']);
        assert.deepEqual(adjustJobHeader('//ABC JOB' + ' '.repeat(100) + '123456'), ['//ABC JOB 123456']);
        assert.deepEqual(adjustJobHeader('//ABC JOB' + ' '.repeat(100) + '123456,USER=USER01'), ['//ABC JOB 123456,USER=USER01']);
        assert.deepEqual(adjustJobHeader('//ABC JOB' + ' '.repeat(100) + '(123456,\'Department\'),USER=USER01'), ['//ABC JOB (123456,\'Department\'),USER=USER01']);
        assert.deepEqual(adjustJobHeader('//ABC JOB' + ' '.repeat(100) + '(123456,,,),USER=USER01'), ['//ABC JOB (123456,,,),USER=USER01']);
        assert.deepEqual(adjustJobHeader('//ABC JOB (' + '1,'.repeat(50) + '\'A\'),USER=USER01 '), [
            '//ABC JOB (1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,',
            '// 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,\'A\'),USER=USER01'
        ]);
    });
});
