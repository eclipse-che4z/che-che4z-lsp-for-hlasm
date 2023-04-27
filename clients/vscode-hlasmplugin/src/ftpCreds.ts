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

export enum connectionSecurityLevel {
    "rejectUnauthorized",
    "acceptUnauthorized",
    "unsecure",
}

export interface ConnectionInfo {
    host: string;
    port: number | undefined;
    user: string;
    password: string;
    hostInput: string;
    securityLevel: connectionSecurityLevel;

    zowe: boolean;
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

export function gatherSecurityLevelFromZowe(profile: any) {
    // tested with Zowe Explorer v2.7.0
    if (profile.secureFtp !== false) {
        if (profile.rejectUnauthorized !== false)
            return connectionSecurityLevel.rejectUnauthorized;
        else
            return connectionSecurityLevel.acceptUnauthorized;
    }
    else
        return connectionSecurityLevel.unsecure;
}

async function gatherConnectionInfoFromZowe(zowe: vscode.Extension<any>, profileName: string): Promise<ConnectionInfo> {
    if (!zowe.isActive)
        await zowe.activate();
    if (!zowe.isActive)
        throw Error("Unable to activate ZOWE Explorer extension");
    const zoweExplorerApi = zowe?.exports;
    const profileCache = zoweExplorerApi.getExplorerExtenderApi().getProfilesCache();
    await profileCache.refresh(zoweExplorerApi);
    const loadedProfile = profileCache.loadNamedProfile(profileName);
    const { host, port, user, password } = loadedProfile.profile;
    const securityLevel = gatherSecurityLevelFromZowe(loadedProfile.profile);

    return { host, port, user, password, hostInput: '@' + profileName, securityLevel, zowe: true };
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
}): Promise<ConnectionInfo> {
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
    return { host, port, user, password, hostInput, securityLevel, zowe: false };
}

const mementoKey = "hlasm.downloadDependencies";

export function getLastRunConfig(context: vscode.ExtensionContext) {
    let lastRun = context.globalState.get(mementoKey, { host: '', user: '', jobcard: '' });
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

export const updateLastRunConfig = (context: vscode.ExtensionContext, lastInput: DownloadDependenciesInputMemento) => context.globalState.update(mementoKey, lastInput);
