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
import { E4E, HLASMExternalFilesEndevor, makeEndevorConfigurationProvider, processChangeNotification } from '../../hlasmExternalFilesEndevor';
import * as vscode from 'vscode';
import { ExternalRequestType } from '../../hlasmExternalFiles';

const dummyProfile = { profile: 'profile', instance: 'instance' };
const profileOnly = {
    getProfileInfo: async () => dummyProfile,
} as any as E4E;
const dummyEvent = <T>(_listener: (e: T) => any, _thisArgs?: any, _disposables?: vscode.Disposable[]): vscode.Disposable => ({ dispose: () => { } });
const endevorMembers = [
    'use_map',
    'environment',
    'stage',
    'system',
    'subsystem',
    'type',
    'element',
    'fingerprint',
];
const datasetMembers = [
    'dataset',
    'member'
];

suite('External files (Endevor)', () => {
    test('Invalid parsing', async () => {
        const client = HLASMExternalFilesEndevor(profileOnly, dummyEvent);

        assert.strictEqual(await client.parseArgs('', ExternalRequestType.list_directory), null);
        assert.strictEqual(await client.parseArgs('', ExternalRequestType.read_file), null);

        assert.strictEqual(await client.parseArgs('/', ExternalRequestType.list_directory), null);
        assert.strictEqual(await client.parseArgs('/', ExternalRequestType.read_file), null);
    });

    test('Endevor type parsing', async () => {
        const client = HLASMExternalFilesEndevor(profileOnly, dummyEvent);

        const args = await client.parseArgs('/profile/map/ENV/STG/SYS/SUB/TYPE', ExternalRequestType.list_directory);

        assert.ok(args);
        assert.deepStrictEqual(endevorMembers.map(x => (<any>args.details)[x]), ['map', 'ENV', 'STG', 'SYS', 'SUB', 'TYPE', undefined, undefined]);
    });

    test('Endevor element parsing', async () => {
        const client = HLASMExternalFilesEndevor(profileOnly, dummyEvent);

        const args = await client.parseArgs('/profile/map/ENV/STG/SYS/SUB/TYPE/ELE.ext', ExternalRequestType.read_file, 'abcdef0123456789');

        assert.ok(args);
        assert.deepStrictEqual(endevorMembers.map(x => (<any>args.details)[x]), ['map', 'ENV', 'STG', 'SYS', 'SUB', 'TYPE', 'ELE', 'abcdef0123456789']);
    });

    test('Dataset parsing', async () => {
        const client = HLASMExternalFilesEndevor(profileOnly, dummyEvent);

        const args = await client.parseArgs('/profile/DATASET.NAME', ExternalRequestType.list_directory);

        assert.ok(args);
        assert.deepStrictEqual(datasetMembers.map(x => (<any>args.details)[x]), ['DATASET.NAME', undefined]);
    });

    test('Dataset member parsing', async () => {
        const client = HLASMExternalFilesEndevor(profileOnly, dummyEvent);

        const args = await client.parseArgs('/profile/DATASET.NAME/MEMBER', ExternalRequestType.read_file);

        assert.ok(args);
        assert.deepStrictEqual(datasetMembers.map(x => (<any>args.details)[x]), ['DATASET.NAME', 'MEMBER']);
    });

    test('List endevor type', async () => {
        const result = [['FILE', '0123456789ABCDEF']];
        const client = HLASMExternalFilesEndevor({ listElements: async (_p: unknown, _t: unknown) => { return result; } } as any as E4E, dummyEvent);

        assert.deepStrictEqual(await client.listMembers({ 'use_map': 'map', normalizedPath: () => '/PATH' } as any, dummyProfile), ['/instance%40profile/PATH/FILE.hlasm?0123456789ABCDEF']);
    });

    test('Read endevor element', async () => {
        const result = ['content', '0123456789ABCDEF'];
        const client = HLASMExternalFilesEndevor({ getElement: async (_p: unknown, _t: unknown) => { return result; } } as any as E4E, dummyEvent);

        assert.deepStrictEqual(await client.readMember({ 'use_map': 'map' } as any, dummyProfile), 'content');
    });

    test('List dataset', async () => {
        const result = ['FILE'];
        const client = HLASMExternalFilesEndevor({ listMembers: async (_p: unknown, _t: unknown) => { return result; } } as any as E4E, dummyEvent);

        assert.deepStrictEqual(await client.listMembers({ normalizedPath: () => '/PATH' } as any, dummyProfile), ['/instance%40profile/PATH/FILE.hlasm']);
    });

    test('Read member', async () => {
        const result = 'content';
        const client = HLASMExternalFilesEndevor({ getMember: async (_p: unknown, _t: unknown) => { return result; } } as any as E4E, dummyEvent);

        assert.deepStrictEqual(await client.readMember({} as any, dummyProfile), result);
    });

    test('Process change notification', async () => {
        let invalidateCalled = false;
        let fired: unknown;
        processChangeNotification([{
            sourceUri: 'a:b',
            environment: 'environment',
            stage: 'stage',
            system: 'system',
            subsystem: 'subsystem',
            type: 'type',
            element: 'element',
            fingerprint: '0123456789ABCDEF',
        }], {
            invalidate() {
                invalidateCalled = true;
            },
            dispose() {

            },
        }, {
            fire: (p) => { fired = p; }, dispose() { }, event: () => ({ dispose() { } }),
        });

        assert.ok(invalidateCalled);
        assert.ok(fired && typeof fired === 'function');
        assert.ok(fired('/TYPE'));
        assert.ok(fired('/TYPE/ELEMENT.hlasm'));
        assert.strictEqual(fired('/SOMETHING'), false);
    });

    test('Endevor configuration provider', async () => {
        const provider = makeEndevorConfigurationProvider({
            isEndevorElement: () => true,
            getProfileInfo: async () => dummyProfile,
            getConfiguration: async () => ({
                pgms: [{
                    program: 'program',
                    pgroup: 'group',
                    options: {
                        "OPTABLE": "UNI"
                    },
                }],
                pgroups: [{
                    name: 'group',
                    libs: [
                        {
                            dataset: 'DATASET',
                        },
                        {
                            environment: 'ENV',
                            stage: '1',
                            system: 'SYS',
                            subsystem: 'SUB',
                            type: 'TYPE',
                        },
                    ],
                    preprocessor: [
                        { name: 'CONWRITE' },
                        { name: 'DFHEAP1$' },
                        {
                            name: 'DSNHPC',
                            options: {
                                'version': 'abcd'
                            }
                        }
                    ],
                    options: {
                        'GOFF': '',
                    },
                }],
            }),
        } as any);

        const result = await provider(vscode.Uri.parse('a:b'));

        assert.deepStrictEqual(result, {
            configuration: {
                name: 'group',
                libs: [
                    {
                        profile: 'instance@profile',
                        dataset: 'DATASET',
                    },
                    {
                        profile: 'instance@profile',
                        environment: 'ENV',
                        stage: '1',
                        system: 'SYS',
                        subsystem: 'SUB',
                        type: 'TYPE',
                    },
                ],
                asm_options: {
                    GOFF: true,
                    OPTABLE: 'UNI',
                },
                preprocessor: [
                    { name: 'ENDEVOR' },
                    {
                        name: 'CICS',
                        options: []
                    },
                    {
                        name: 'DB2',
                        options: {
                            conditional: true,
                            VERSION: 'abcd'
                        }
                    }
                ],
            },
        });
    });
});
