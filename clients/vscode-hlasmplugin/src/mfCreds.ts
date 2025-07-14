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

import * as vscode from 'vscode';
import { askUser, pickUser } from './uiUtils';
import { AccessOptions } from 'basic-ftp';
import { MementoKey } from './mementoKeys';
import { AsyncMutex } from './asyncMutex';

export enum connectionSecurityLevel {
    "rejectUnauthorized",
    "acceptUnauthorized",
    "unsecure",
}

export type FtpConnectionInfo = {
    host: string;
    port: number | undefined;
    user: string;
    password: string;
    hostInput: string;
    securityLevel: connectionSecurityLevel;
}

export type ZoweConnectionInfo = {
    loadedProfile: any;
    profileCache: any;
    zoweExplorerApi: any;
    user: string;
    hostInput: string;
}

export function translateConnectionInfo(connection: {
    host: string;
    port?: number;
    user: string;
    password: string;
    securityLevel: connectionSecurityLevel
}): AccessOptions {
    return {
        host: connection.host,
        user: connection.user,
        password: connection.password,
        port: connection.port,
        secure: connection.securityLevel !== connectionSecurityLevel.unsecure,
        secureOptions: connection.securityLevel === connectionSecurityLevel.unsecure ? undefined : { rejectUnauthorized: connection.securityLevel !== connectionSecurityLevel.acceptUnauthorized }
    }
}

async function gatherConnectionInfoFromZowe(zowe: vscode.Extension<any>, profileName: string): Promise<ZoweConnectionInfo> {
    if (!zowe.isActive)
        await zowe.activate();
    if (!zowe.isActive)
        throw Error("Unable to activate ZOWE Explorer extension");
    const profileType = 'zosmf'; // No default `type` available?
    const zoweExplorerApi = zowe?.exports;
    const profileCache = zoweExplorerApi.getExplorerExtenderApi().getProfilesCache();
    await profileCache.refresh(zoweExplorerApi);
    if (profileName === '') {
        profileName = profileCache.getDefaultProfile(profileType);
    }
    const loadedProfile = profileCache.loadNamedProfile(profileName);

    return { loadedProfile, profileCache, zoweExplorerApi, user: loadedProfile.user, hostInput: '@' + profileName };
}

const securityOptions = Object.freeze([
    { label: "Use TLS, reject unauthorized certificated", value: connectionSecurityLevel.rejectUnauthorized },
    { label: "Use TLS, accept unauthorized certificated", value: connectionSecurityLevel.acceptUnauthorized },
    { label: "Unsecured connection", value: connectionSecurityLevel.unsecure },
]);

export async function gatherConnectionInfo(lastInput: {
    host: string;
    user: string;
    jobcard: string;
}): Promise<FtpConnectionInfo | ZoweConnectionInfo> {
    const zowe = vscode.extensions.getExtension("Zowe.vscode-extension-for-zowe");

    const hostInput = await askUser(zowe ? "host[:port] or @zowe-profile-name" : "host[:port]", false, !zowe && lastInput.host.startsWith('@') ? '' : lastInput.host);
    const hostPort = hostInput.split(':');
    if (hostPort.length < 1 || hostPort.length > 2)
        throw Error("Invalid hostname or port");

    const host = hostPort[0];
    const port = hostPort.length > 1 ? +hostPort[1] : undefined;
    if (zowe && port === undefined && host.startsWith('@'))
        return gatherConnectionInfoFromZowe(zowe, host.slice(1));

    const user = await askUser("user name", false, lastInput.user);
    const password = await askUser("password", true);
    const securityLevel = await pickUser("Select security option", securityOptions);
    return { host, port, user, password, hostInput, securityLevel };
}

export function getLastRunConfig(context: vscode.ExtensionContext) {
    let lastRun = context.globalState.get(MementoKey.DownloadDependencies, { host: '', user: '', jobcard: '' });
    return {
        host: '' + (lastRun.host || ''),
        user: '' + (lastRun.user || ''),
        jobcard: '' + (lastRun.jobcard || ''),
    };
}

interface DownloadDependenciesInputMemento {
    host: string;
    user: string;
    jobcard: string;
};

export const updateLastRunConfig = (context: vscode.ExtensionContext, lastInput: DownloadDependenciesInputMemento) => context.globalState.update(MementoKey.DownloadDependencies, lastInput);

const zoweClientLock = new AsyncMutex();

export async function ensureValidMfZoweClient<R extends { getSession(): unknown; }>(info: ZoweConnectionInfo, apiGetter: (profile: unknown) => R): Promise<R> {
    // it looks like there is a race in checkCurrentProfile...
    return zoweClientLock.locked(async () => {
        const { status } = await info.profileCache.checkCurrentProfile(info.loadedProfile);

        if (status !== 'active')
            throw Error('Zowe profile is not active');

        return apiGetter.call(info.zoweExplorerApi, info.loadedProfile);
    });
}
