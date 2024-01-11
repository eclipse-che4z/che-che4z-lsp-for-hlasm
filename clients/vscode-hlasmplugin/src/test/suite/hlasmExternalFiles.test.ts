/*
 * Copyright (c) 2023 Broadcom.
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
import * as crypto from "crypto";
import { ClientUriDetails, ExternalFilesInvalidationdata, ExternalRequestType, HLASMExternalFiles } from '../../hlasmExternalFiles';
import { EventEmitter, FileSystem, Uri } from 'vscode';
import { FileType } from 'vscode';
import { TextEncoder } from 'util';
import { deflateSync } from 'zlib';

suite('External files', () => {

    test('Invalid messages', async () => {
        const ext = new HLASMExternalFiles('test', {
            onNotification: (_, __) => { return { dispose: () => { } }; },
            sendNotification: (_: any, __: any) => Promise.resolve(),
        }, {
            readFile: async (_: Uri) => { throw Error('not found'); }
        } as any as FileSystem);

        assert.strictEqual(await ext.handleRawMessage(null), null);
        assert.strictEqual(await ext.handleRawMessage(undefined), null);
        assert.strictEqual(await ext.handleRawMessage({}), null);
        assert.strictEqual(await ext.handleRawMessage(5), null);
        assert.strictEqual(await ext.handleRawMessage({ id: 'id', op: '' }), null);
        assert.strictEqual(await ext.handleRawMessage({ id: 5, op: 5 }), null);

        assert.deepStrictEqual(await ext.handleRawMessage({ id: 5, op: '' }), { id: 5, error: { code: -5, msg: 'Invalid request' } });
        assert.deepStrictEqual(await ext.handleRawMessage({ id: 5, op: 'read_file', url: 5 }), { id: 5, error: { code: -5, msg: 'Invalid request' } });
        assert.deepStrictEqual(await ext.handleRawMessage({ id: 5, op: 'read_file', url: {} }), { id: 5, error: { code: -5, msg: 'Invalid request' } });

        assert.deepStrictEqual(await ext.handleRawMessage({ id: 5, op: 'read_file', url: 'unknown:scheme' }), { id: 5, error: { code: -1000, msg: 'not found' } });

        assert.deepStrictEqual(await ext.handleRawMessage({ id: 5, op: 'read_file', url: 'test:/SERVICE' }), { id: 5, error: { code: -1000, msg: 'No client' } });
    });

    test('Clear cache', async () => {
        const cacheUri = Uri.parse('test:cache/');

        let readCounter = 0;
        let deleteCounter = 0;

        const ext = new HLASMExternalFiles('test', {
            onNotification: (_, __) => { return { dispose: () => { } }; },
            sendNotification: (_: any, __: any) => Promise.resolve(),
        }, {
            readDirectory: async (uri: Uri) => {
                ++readCounter;
                assert.strictEqual(cacheUri.toString(), uri.toString());
                return [['A', FileType.File]];
            },
            delete: async (uri: Uri, options?: { recursive?: boolean; useTrash?: boolean }) => {
                ++deleteCounter;
                assert.strictEqual(uri.toString(), Uri.joinPath(cacheUri, 'A').toString())
            },
        } as any as FileSystem, {
            uri: cacheUri,

        });

        await ext.clearCache();

        assert.strictEqual(readCounter, 1);
        assert.strictEqual(deleteCounter, 1);

    });
    const nameGenerator = (components: string[], service: string = 'TEST') => {
        return 'v3.' + service + '.' + crypto.createHash('sha256').update(JSON.stringify(components)).digest().toString('hex');
    };
    test('Access cached content', async () => {
        const cacheUri = Uri.parse('test:cache/');
        const dirResponse = deflateSync(new TextEncoder().encode(JSON.stringify({ normalizedPath: '/DIR', not_exists: true })));
        const dir2Content = ['/DIR2/FILE'];
        const dir2Response = deflateSync(new TextEncoder().encode(JSON.stringify({ normalizedPath: '/DIR2', data: dir2Content })));
        const fileContent = 'file content';
        const fileResponse = deflateSync(new TextEncoder().encode(JSON.stringify({ normalizedPath: '/DIR2/FILE', data: fileContent })));

        let dirWritten = false;
        let dir2Written = false;
        let fileWritten = false;

        const ext = new HLASMExternalFiles('test', {
            onNotification: (_, __) => { return { dispose: () => { } }; },
            sendNotification: (_: any, __: any) => Promise.resolve(),
        }, {
            readFile: async (uri: Uri) => {
                const filename = uri.path.split('/').pop();
                if (filename === nameGenerator(['SERVER', '/DIR']))
                    return dirResponse;
                if (filename === nameGenerator(['SERVER', '/DIR2']))
                    return dir2Response;
                if (filename === nameGenerator(['SERVER', '/DIR2/FILE']))
                    return fileResponse;
                assert.ok(false);
            },
            writeFile: async (uri: Uri, content: Uint8Array) => {
                const filename = uri.path.split('/').pop();
                if (filename === nameGenerator(['SERVER', '/DIR'])) {
                    dirWritten = true;
                    assert.deepStrictEqual(content, dirResponse);
                    return;
                }
                if (filename === nameGenerator(['SERVER', '/DIR2'])) {
                    dir2Written = true;
                    assert.deepStrictEqual(content, dir2Response);
                    return;
                }
                if (filename === nameGenerator(['SERVER', '/DIR2/FILE'])) {
                    fileWritten = true;
                    assert.deepStrictEqual(content, fileResponse);
                    return;
                }
                assert.ok(false);
            },
        } as any as FileSystem, {
            uri: cacheUri,

        });

        ext.setClient('TEST', {
            parseArgs: async (path: string, purpose: ExternalRequestType) => {
                if (purpose === ExternalRequestType.read_file)
                    return {
                        details: {
                            normalizedPath: () => '/DIR2/FILE',
                        },
                        server: undefined,
                    };
                else if (path.endsWith('DIR'))
                    return {
                        details: {
                            normalizedPath: () => '/DIR',
                        },
                        server: undefined,
                    };
                else if (path.endsWith('DIR2'))
                    return {
                        details: {
                            normalizedPath: () => '/DIR2',
                        },
                        server: undefined,
                    };
                return null;
            },

            listMembers: (_: ClientUriDetails) => Promise.resolve(null),
            readMember: (_: ClientUriDetails) => Promise.resolve(null),

            serverId: async () => 'SERVER',
        });

        const dir = await ext.handleRawMessage({ id: 4, op: 'list_directory', url: 'test:/TEST/DIR' });
        assert.ok(dir);
        assert.strictEqual(dir.id, 4);
        assert.ok('error' in dir);
        assert.strictEqual(dir?.error?.code, 0);

        const dir2 = await ext.handleRawMessage({ id: 5, op: 'list_directory', url: 'test:/TEST/DIR2' });
        assert.ok(dir2);
        assert.strictEqual(dir2.id, 5);
        assert.ok('data' in dir2);
        assert.ok(dir2.data instanceof Object);
        assert.deepStrictEqual(dir2.data.member_urls, ['test:/TEST/DIR2/FILE']);

        const file = await ext.handleRawMessage({ id: 6, op: 'read_file', url: 'test:/TEST/DIR2/FILE' });
        assert.ok(file);
        assert.strictEqual(file.id, 6);
        assert.ok('data' in file);
        assert.strictEqual(file.data, fileContent);

        const fileh = await ext.handleRawMessage({ id: 6, op: 'read_file', url: 'test:/TEST/DIR2/FILE' });
        assert.ok(fileh);
        assert.strictEqual(fileh.id, 6);
        assert.ok('data' in fileh);
        assert.strictEqual(fileh.data, fileContent);

        ext.dispose();

        assert.strictEqual(dirWritten, true);
        assert.strictEqual(dir2Written, true);
        assert.strictEqual(fileWritten, true);
    });

    test('Access invalid cache content', async () => {
        const ext = new HLASMExternalFiles('test', {
            onNotification: (_, __) => { return { dispose: () => { } }; },
            sendNotification: (_: any, __: any) => Promise.resolve(),
        }, {
            readFile: async (_: Uri) => Uint8Array.from([0]),
            writeFile: async (_uri: Uri, _content: Uint8Array) => { },
        } as any as FileSystem, {
            uri: Uri.parse('test:cache/')
        });

        ext.setClient('TEST', {
            parseArgs: async (_path: string, _purpose: ExternalRequestType) => {
                return {
                    details: {
                        normalizedPath: () => '/DIR',
                    },
                    server: undefined,
                };
            },

            listMembers: (_: ClientUriDetails) => Promise.resolve(null),
            readMember: (_: ClientUriDetails) => Promise.resolve(null),
        });

        const dir = await ext.handleRawMessage({ id: 4, op: 'list_directory', url: 'test:/TEST/DIR' });
        assert.ok(dir);
        assert.strictEqual(dir.id, 4);
        assert.ok('error' in dir);
        assert.strictEqual(dir?.error?.code, 0);
    });

    test('Selective cache clear', async () => {
        const cacheUri = Uri.parse('test:cache/');

        let resolve: () => void;
        const deletePromise = new Promise<void>((r) => { resolve = r; });

        const ext = new HLASMExternalFiles('test', {
            onNotification: (_, __) => { return { dispose: () => { } }; },
            sendNotification: (_: any, __: any) => Promise.resolve(),
        }, {
            readDirectory: async (uri: Uri) => {
                assert.strictEqual(cacheUri.toString(), uri.toString());
                return [[nameGenerator(['SERVER', 'B'], 'TEST2'), FileType.File], [nameGenerator(['SERVER', 'A']), FileType.File]];
            },
            delete: async (uri: Uri, options?: { recursive?: boolean; useTrash?: boolean }) => {
                assert.strictEqual(uri.toString(), Uri.joinPath(cacheUri, nameGenerator(['SERVER', 'A'])).toString())
                resolve();
            },
        } as any as FileSystem, {
            uri: cacheUri,
        });

        const emmiter = new EventEmitter<ExternalFilesInvalidationdata | undefined>();
        ext.setClient('TEST', {
            parseArgs: async (path: string, purpose: ExternalRequestType) => null,
            listMembers: (_: ClientUriDetails) => Promise.resolve(null),
            readMember: (_: ClientUriDetails) => Promise.resolve(null),

            invalidate: emmiter.event,
        });

        emmiter.fire({ paths: ['A'], serverId: 'SERVER' });

        await deletePromise;

    });

    test('Selective cache clear with predicate', async () => {
        const cacheUri = Uri.parse('test:cache/');

        let resolve: () => void;
        const deletePromise = new Promise<void>((r) => { resolve = r; });

        const ext = new HLASMExternalFiles('test', {
            onNotification: (_, __) => { return { dispose: () => { } }; },
            sendNotification: (_: any, __: any) => Promise.resolve(),
        }, {
            readDirectory: async (uri: Uri) => {
                assert.strictEqual(cacheUri.toString(), uri.toString());
                return [[nameGenerator(['SERVER', 'B'], 'TEST2'), FileType.File], [nameGenerator(['SERVER', 'A']), FileType.File]];
            },
            readFile: async (uri: Uri) => {
                if (uri.path.endsWith(nameGenerator(['SERVER', 'A'])))
                    return deflateSync(new TextEncoder().encode(JSON.stringify({ normalizedPath: '/SERVER/A' })));
                else
                    return deflateSync(new TextEncoder().encode(JSON.stringify({ normalizedPath: '/SERVER/B' })));
            },
            delete: async (uri: Uri, options?: { recursive?: boolean; useTrash?: boolean }) => {
                assert.strictEqual(uri.toString(), Uri.joinPath(cacheUri, nameGenerator(['SERVER', 'A'])).toString())
                resolve();
            },
        } as any as FileSystem, {
            uri: cacheUri,
        });

        const emmiter = new EventEmitter<ExternalFilesInvalidationdata | undefined>();
        ext.setClient('TEST', {
            parseArgs: async (path: string, purpose: ExternalRequestType) => null,
            listMembers: (_: ClientUriDetails) => Promise.resolve(null),
            readMember: (_: ClientUriDetails) => Promise.resolve(null),

            invalidate: emmiter.event,
        });

        emmiter.fire((p) => { return p.endsWith('/A') });

        await deletePromise;

    });
});
