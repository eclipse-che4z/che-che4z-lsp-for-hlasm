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

import * as vscode from 'vscode';
import { ExternalRequestType, HlasmExtension, ExternalFilesInvalidationdata } from './extension.interface';
import { AsmOptions, ConfigurationProviderRegistration, Preprocessor } from './hlasmExternalConfigurationProvider';

interface EndevorType {
    use_map: string,
    environment: string,
    stage: string,
    system: string,
    subsystem: string,
    type: string,
    normalizedPath: () => string,
    toDisplayString: () => string,
    serverId?: () => string | undefined,
};
interface EndevorElement {
    use_map: string,
    environment: string,
    stage: string,
    system: string,
    subsystem: string,
    type: string,
    element: string,
    fingerprint: string,
    normalizedPath: () => string,
    toDisplayString: () => string,
    serverId?: () => string | undefined,
};
interface EndevorDataset {
    dataset: string,
    normalizedPath: () => string,
    toDisplayString: () => string,
    serverId?: () => string | undefined,
};

interface EndevorMember {
    dataset: string,
    member: string,
    normalizedPath: () => string,
    toDisplayString: () => string,
    serverId?: () => string | undefined,
};

type TypeOrArray<T> = T | T[];
function asArray<T>(o: TypeOrArray<T>) {
    if (Array.isArray(o)) return o;
    return [o];
}

type E4EExternalConfigurationResponse = {
    pgms: ReadonlyArray<{
        program: string;
        pgroup: string;
        options?: {
            [key: string]: string;
        },
    }>;
    pgroups: ReadonlyArray<{
        name: string;
        libs: (
            {
                dataset: string;
                optional?: boolean;
                profile?: string;
            } | {
                environment: string;
                stage: string;
                system: string;
                subsystem: string;
                type: string;
                use_map?: boolean;
                optional?: boolean;
                profile?: string;
            })[];
        options?: {
            [key: string]: string;
        },
        preprocessor?: TypeOrArray<{
            name: string,
            options?: {
                [key: string]: string
            }
        }>;
    }>;
};

type Filename = string;
type Fingerprint = string;
type MemberName = string;
type Content = string;

type ResolvedProfile = { profile: string; instance: string };

type ElementInfo = {
    sourceUri?: string;
    environment: string;
    stage: string;
    system: string;
    subsystem: string;
    type: string;
    element: string;
    fingerprint?: string;
};

type ExternalConfigurationOptions = {
    type: 'HLASM';
};

export interface E4E {
    isEndevorElement: (uri: string) => boolean;
    getProfileInfo: (
        uriStringOrPartialProfile: Partial<ResolvedProfile> | string
    ) => Promise<ResolvedProfile | Error>;
    listElements: (
        profile: ResolvedProfile,
        type: {
            use_map: boolean;
            environment: string;
            stage: string;
            system: string;
            subsystem: string;
            type: string;
        }
    ) => Promise<[Filename, Fingerprint][] | Error>;
    getElement: (
        profile: ResolvedProfile,
        type: {
            use_map: boolean;
            environment: string;
            stage: string;
            system: string;
            subsystem: string;
            type: string;
            element: string;
            fingerprint: string;
        }
    ) => Promise<[Content, Fingerprint] | Error>;
    listMembers: (
        profile: ResolvedProfile,
        type: {
            dataset: string;
        }
    ) => Promise<MemberName[] | Error>;
    getMember: (
        profile: ResolvedProfile,
        type: {
            dataset: string;
            member: string;
        }
    ) => Promise<Content | Error>;
    getConfiguration: (
        sourceUri: string,
        options: ExternalConfigurationOptions
    ) => Promise<E4EExternalConfigurationResponse | Error>;
    onDidChangeElement: vscode.Event<ElementInfo[]>;
}

const nameof = <T>(name: keyof T) => name;

function validateE4E(e4e: any): e4e is E4E {
    const valid = e4e instanceof Object &&
        nameof<E4E>('listElements') in e4e &&
        nameof<E4E>('getElement') in e4e &&
        nameof<E4E>('listMembers') in e4e &&
        nameof<E4E>('getMember') in e4e &&
        nameof<E4E>('isEndevorElement') in e4e &&
        nameof<E4E>('getProfileInfo') in e4e &&
        nameof<E4E>('getConfiguration') in e4e &&
        nameof<E4E>('onDidChangeElement') in e4e;
    if (!valid)
        throw Error('incompatible interface');
    return valid;
}

function addStringOptions(o: any, kv: Map<string, [string, string]>, list: string[]) {
    let added = false;
    for (const k of list) {
        if (kv.has(k)) {
            o[k] = kv.get(k)?.[1];
            added = true;
        }
    }
    return added;
}
function addBooleanOptions(o: any, kv: Map<string, [string, string]>, list: string[]) {
    let added = false;
    for (const k of list) {
        if (kv.has(k)) {
            o[k] = true;
            added = true;
        }
        else if (kv.has('NO' + k)) {
            o[k] = false;
            added = true;
        }
    }
    return added;
}
function generateOptionMap(obj: { [key: string]: string }) {
    return new Map(Object.keys(obj).map(o => [o.toUpperCase(), [o, obj[o]] as [string, string]]));
}

function translateAsmOptions(opts?: { [key: string]: string }): AsmOptions | undefined {
    if (!opts) return undefined;

    let added = false;

    const kv = generateOptionMap(opts);

    const result: any = {};

    added ||= addStringOptions(result, kv, ['SYSPARM', 'PROFILE', 'SYSTEM_ID', 'MACHINE', 'OPTABLE']);
    added ||= addBooleanOptions(result, kv, ['GOFF', 'XOBJECT']);

    return added ? result : undefined;
}

const translatePreprocessor: { [key: string]: 'DB2' | 'ENDEVOR' | 'CICS' } = Object.freeze({
    'DSNHPC': 'DB2',
    'PBLHPC': 'DB2',
    'CONWRITE': 'ENDEVOR',
    'DFHEAP1$': 'CICS',
});

function translatePreprocessors(input: undefined | TypeOrArray<{
    name: string,
    options?: {
        [key: string]: string
    }
}>): Preprocessor[] | undefined {
    if (!input) return undefined;
    const prep = asArray(input);

    const result: Preprocessor[] = [];

    for (const p of prep) {
        const type = translatePreprocessor[p.name.toUpperCase()];
        if (!type) continue; // just skip?

        if (type === 'DB2') {
            const prep: Preprocessor = {
                name: 'DB2',
                options: {
                    conditional: true, // TODO: no detection available yet
                }
            };
            if (p.options)
                addStringOptions(prep.options, generateOptionMap(p.options), ['VERSION']);
            result.push(prep);
        }
        else if (type === 'CICS') {
            const prep: Preprocessor = {
                name: 'CICS',
                options: [],
            };
            if (p.options)
                addBooleanOptions(prep, generateOptionMap(p.options), ["PROLOG", "EPILOG", "LEASM"]);

            result.push(prep);
        }
        else if (type === 'ENDEVOR') {
            result.push({ name: 'ENDEVOR' });
        }
    }

    if (result.length === 0)
        return undefined;

    return result;
}

function translateLibs(x: string, profile: string): string;
function translateLibs<T extends object>(x: T, profile: string): T & { profile: string };
function translateLibs(x: string | object, profile: string) {
    if (typeof x === 'string') return x;
    return { ...x, profile };
}

function parseEndevorType(server: ResolvedProfile, args: string[/*6*/]) {
    const [use_map, environment, stage, system, subsystem, type] = args;
    const path = `/${encodeURIComponent(use_map)}/${encodeURIComponent(environment)}/${encodeURIComponent(stage)}/${encodeURIComponent(system)}/${encodeURIComponent(subsystem)}/${encodeURIComponent(type)}`;
    return {
        details: {
            use_map,
            environment,
            stage,
            system,
            subsystem,
            type,
            normalizedPath: () => path,
            toDisplayString: () => `${use_map}/${environment}/${stage}/${system}/${subsystem}/${type}`,
        },
        server
    };
}

function parserEndevorElement(server: ResolvedProfile, args: string[/*7*/], query: string | undefined) {
    const [use_map, environment, stage, system, subsystem, type, element_hlasm] = args;

    const [element] = element_hlasm.split('.');
    if (element.length === 0) return null;
    const fingerprint = query?.match(/^([a-zA-Z0-9]+)$/)?.[1];
    const q = fingerprint ? '?' + query : '';
    const path = `/${encodeURIComponent(use_map)}/${encodeURIComponent(environment)}/${encodeURIComponent(stage)}/${encodeURIComponent(system)}/${encodeURIComponent(subsystem)}/${encodeURIComponent(type)}/${encodeURIComponent(element)}.hlasm${q}`;
    return {
        details: {
            use_map,
            environment,
            stage,
            system,
            subsystem,
            type,
            element,
            fingerprint,
            normalizedPath: () => path,
            toDisplayString: () => `${use_map}/${environment}/${stage}/${system}/${subsystem}/${type}/${element}`,
            serverId: () => server.instance,
        },
        server
    };
}

function parseDataset(server: ResolvedProfile, args: string[/*1*/]) {
    const [dataset] = args;
    return {
        details: {
            dataset,
            normalizedPath: () => `/${encodeURIComponent(dataset)}`,
            toDisplayString: () => `${dataset}`,
        },
        server,
    };

}

function parseMember(server: ResolvedProfile, args: string[/*2*/]) {
    const [dataset, memeber_hlasm] = args;
    const [member] = memeber_hlasm.split('.');
    if (member.length === 0) return null;
    return {
        details: {
            dataset,
            member,
            normalizedPath: () => `/${encodeURIComponent(dataset)}/${encodeURIComponent(member)}.hlasm`,
            toDisplayString: () => `${dataset}(${member})`,
            serverId: () => server.instance,
        },
        server,
    };
}

function profileAsString(profile: ResolvedProfile) {
    return `${profile.instance}@${profile.profile}`;
}

function listEndevorElements(e4e: E4E, type_spec: EndevorType, profile: ResolvedProfile) {
    return e4e.listElements(profile, {
        use_map: type_spec.use_map === "map",
        environment: type_spec.environment,
        stage: type_spec.stage,
        system: type_spec.system,
        subsystem: type_spec.subsystem,
        type: type_spec.type
    }).then(
        r => r instanceof Error ? Promise.reject(r) : r?.map(([file, fingerprint]) => `/${profileAsString(profile)}${type_spec.normalizedPath()}/${encodeURIComponent(file)}.hlasm?${fingerprint.toString()}`) ?? null
    );
}

function readEndevorElement(e4e: E4E, file_spec: EndevorElement, profile: ResolvedProfile) {
    return e4e.getElement(profile, {
        use_map: file_spec.use_map === "map",
        environment: file_spec.environment,
        stage: file_spec.stage,
        system: file_spec.system,
        subsystem: file_spec.subsystem,
        type: file_spec.type,
        element: file_spec.element,
        fingerprint: file_spec.fingerprint,
    }).then(r => r instanceof Error ? Promise.reject(r) : r[0]);
}

function listEndevorMembers(e4e: E4E, type_spec: EndevorDataset, profile: ResolvedProfile) {
    return e4e.listMembers(profile, {
        dataset: type_spec.dataset
    }).then(
        r => r instanceof Error ? Promise.reject(r) : r?.map((member) => `/${profileAsString(profile)}${type_spec.normalizedPath()}/${encodeURIComponent(member)}.hlasm`) ?? null
    );
}

function readEndevorMember(e4e: E4E, file_spec: EndevorMember, profile: ResolvedProfile) {
    return e4e.getMember(profile, {
        dataset: file_spec.dataset,
        member: file_spec.member,
    }).then(r => r instanceof Error ? Promise.reject(r) : r);
}

function whitespaceAsUndefined(s: string) {
    for (const c of s)
        if (c !== ' ') return s;
    return undefined;
}

function asPartialProfile(s: string): Partial<ResolvedProfile> {
    const idx = s.indexOf('@');
    if (idx === -1)
        return { instance: whitespaceAsUndefined(s), profile: undefined };
    else
        return { instance: whitespaceAsUndefined(s.substring(0, idx)), profile: whitespaceAsUndefined(s.substring(idx + 1)) };
}

export function HLASMExternalFilesEndevor(e4e: E4E, invalidate: vscode.Event<ExternalFilesInvalidationdata | undefined>) {
    return {
        parseArgs: async (p: string, purpose: ExternalRequestType, query?: string) => {
            const args = p.split('/').slice(1).map(decodeURIComponent);
            if (args.length === 0) return null;

            const profile = await e4e.getProfileInfo(asPartialProfile(args[0]));
            if (profile instanceof Error) throw p;

            if (purpose === ExternalRequestType.list_directory && args.length === 7)
                return parseEndevorType(profile, args.slice(1));

            if (purpose === ExternalRequestType.list_directory && args.length === 2)
                return parseDataset(profile, args.slice(1));

            if (purpose === ExternalRequestType.read_file && args.length === 8)
                return parserEndevorElement(profile, args.slice(1), query);

            if (purpose === ExternalRequestType.read_file && args.length === 3)
                return parseMember(profile, args.slice(1));

            return null;
        },

        listMembers: (type_spec: EndevorType | EndevorDataset, profile: ResolvedProfile) => {
            if ('use_map' in type_spec)
                return listEndevorElements(e4e, type_spec, profile);
            else
                return listEndevorMembers(e4e, type_spec, profile);
        },

        readMember: async (file_spec: EndevorElement | EndevorMember, profile: ResolvedProfile) => {
            if ('use_map' in file_spec)
                return readEndevorElement(e4e, file_spec, profile);
            else
                return readEndevorMember(e4e, file_spec, profile);
        },

        invalidate,
    }
}

export function makeEndevorConfigurationProvider(e4e: E4E) {
    return async (uri: vscode.Uri) => {
        const uriString = uri.toString();
        if (!e4e.isEndevorElement(uriString)) return null;
        const profile = await e4e.getProfileInfo(uriString);
        if (profile instanceof Error) throw profile;

        const result = await e4e.getConfiguration(uriString, { type: 'HLASM' });
        if (result instanceof Error) throw result;
        const candidate = result.pgroups.find(x => x.name === result.pgms[0].pgroup);
        if (!candidate) throw Error('Invalid configuration');
        return {
            configuration: {
                name: candidate.name,
                libs: candidate.libs.map(x => translateLibs(x, profileAsString(profile))),
                asm_options: {
                    ...translateAsmOptions(candidate.options),
                    ...translateAsmOptions(result.pgms[0].options),
                },
                preprocessor: translatePreprocessors(candidate.preprocessor),
            }
        };
    };
}

const typeExtract = /\/([^/.]+)$/;
const elementExtract = /\/([^/]+\/[^/]+\.hlasm)/;

export function processChangeNotification(elements: ElementInfo[], cp: ConfigurationProviderRegistration, cacheInvalidationEmitter: vscode.EventEmitter<ExternalFilesInvalidationdata | undefined>) {
    const uniqueType = new Set<string>();
    const uniqueElement = new Set<string>();
    for (const e of elements) {
        if (e.sourceUri)
            cp.invalidate(vscode.Uri.parse(e.sourceUri));
        if (e.type)
            uniqueType.add(encodeURIComponent(e.type).toLowerCase());
        if (e.type && e.element)
            uniqueElement.add(`${encodeURIComponent(e.type)}/${encodeURIComponent(e.element)}.hlasm`.toLowerCase());
    }

    cacheInvalidationEmitter.fire((p) => {
        const typeInfo = typeExtract.exec(p);
        const eleInfo = elementExtract.exec(p);
        let result = false;
        if (typeInfo)
            result ||= uniqueType.has(typeInfo[1].toLowerCase());

        if (eleInfo)
            result ||= uniqueElement.has(eleInfo[1].toLowerCase());

        return result;
    });
}

function performRegistration(ext: HlasmExtension, e4e: E4E) {
    const invalidationEventEmmiter = new vscode.EventEmitter<ExternalFilesInvalidationdata | undefined>();

    const extFiles = ext.registerExternalFileClient('ENDEVOR', HLASMExternalFilesEndevor(e4e, invalidationEventEmmiter.event));

    const cp = ext.registerExternalConfigurationProvider(makeEndevorConfigurationProvider(e4e));

    e4e.onDidChangeElement((elements) => processChangeNotification(elements, cp, invalidationEventEmmiter));

    return { dispose: () => { extFiles.dispose(); cp.dispose(); } };
}

function findE4EAndRegister(ext: HlasmExtension, subscriptions: vscode.Disposable[], outputChannel: vscode.OutputChannel) {
    return !!vscode.extensions.getExtension('broadcommfd.explorer-for-endevor')?.activate()
        .then(e4e => e4e && validateE4E(e4e) && subscriptions.push(performRegistration(ext, e4e)))
        .then(undefined, e => outputChannel.appendLine(`ERROR: Explorer for Endevor integration failed - ${e?.message ?? e}`));
}

export function handleE4EIntegration(ext: HlasmExtension, subscriptions: vscode.Disposable[], outputChannel: vscode.OutputChannel) {
    if (findE4EAndRegister(ext, subscriptions, outputChannel)) return;
    let listener: vscode.Disposable | null = vscode.extensions.onDidChange(() => {
        if (!findE4EAndRegister(ext, subscriptions, outputChannel)) return;
        listener?.dispose();
        listener = null;
    });
    subscriptions.push({ dispose: () => { listener?.dispose(); listener = null; } });
}
