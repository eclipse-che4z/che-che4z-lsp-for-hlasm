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

import * as vscode from 'vscode';
import { fork } from 'child_process';
import { Client, FTPResponse, FileInfo, FTPError } from 'basic-ftp'
import { Readable, Writable } from 'stream';
import { EOL, homedir } from 'os';
import path = require('node:path');
import { promises as fsp } from "fs";
import { hlasmplugin_folder, proc_grps_file } from './constants';
import { Telemetry } from './telemetry';

const cancelMessage = "Action was cancelled";

export type JobId = string;
export interface JobDescription {
    jobname: string;
    id: JobId;
    details: string;
}

function getJobDetailInfo(job: JobDescription): { rc: number; spoolFiles: number } | undefined {
    const parsed = /^.*RC=(\d+)\s+(\d+) spool file/.exec(job.details);
    if (!parsed)
        return undefined;
    else
        return { rc: +parsed[1], spoolFiles: +parsed[2] };
}
export interface JobClient {
    submitJcl(jcl: string): Promise<JobId>;
    setListMask(mask: string): Promise<void>;
    list(): Promise<JobDescription[]>;
    download(target: Writable | string, id: JobId, spoolFile: number): Promise<void>;
    dispose(): void;
}

async function basicFtpJobClient(connection: {
    host: string;
    port?: number;
    user: string;
    password: string;
    securityLevel: connectionSecurityLevel
}): Promise<JobClient> {
    const client = new Client();
    client.parseList = (rawList: string): FileInfo[] => {
        return rawList.split(/\r?\n/).slice(1).filter(x => !/^\s*$/.test(x)).map((value) => new FileInfo(value));
    };
    await client.access({
        host: connection.host,
        user: connection.user,
        password: connection.password,
        port: connection.port,
        secure: connection.securityLevel !== connectionSecurityLevel.unsecure,
        secureOptions: connection.securityLevel === connectionSecurityLevel.unsecure ? undefined : { rejectUnauthorized: connection.securityLevel !== connectionSecurityLevel.rejectUnauthorized }
    });

    const checkResponse = (resp: FTPResponse) => {
        if (resp.code < 200 || resp.code > 299)
            throw Error("FTP Error: " + resp.message);
    }
    const checkedCommand = async (command: string): Promise<string> => {
        const resp = await client.send(command);
        checkResponse(resp);
        return resp.message
    }
    const switchText = async () => { await checkedCommand("TYPE A"); }
    const switchBinary = async () => { await checkedCommand("TYPE I"); }

    await checkedCommand("SITE FILE=JES");
    return {
        async submitJcl(jcl: string): Promise<string> {
            await switchText();
            const jobUpload = await client.uploadFrom(Readable.from(jcl), "JOB");
            checkResponse(jobUpload);
            const jobid = /^.*as ([Jj](?:[Oo][Bb])?\d+)/.exec(jobUpload.message);
            if (!jobid)
                throw Error("Unable to extract the job id");
            return jobid[1];
        },
        async setListMask(mask: string): Promise<void> {
            await checkedCommand("SITE JESJOBNAME=" + mask);
            await checkedCommand("SITE JESSTATUS=OUTPUT");
        },
        async list(): Promise<JobDescription[]> {
            try {
                await switchText();
                return (await client.list()).map((x: FileInfo): JobDescription => {
                    const parsedLine = /(\S+)\s+(\S+)\s+(.*)/.exec(x.name);
                    if (!parsedLine)
                        throw Error("Unable to parse the job list");
                    return { jobname: parsedLine[1], id: parsedLine[2], details: parsedLine[3] }
                });
            }
            catch (e) {
                if (e instanceof FTPError && e.code == 550)
                    return [];
                throw e;
            }
        },
        async download(target: string | Writable, id: JobId, spoolFile: number): Promise<void> {
            await switchBinary();
            checkResponse(await client.downloadTo(target, id + "." + spoolFile));
        },
        dispose(): void {
            client.close();
        }
    };
}

interface JobDetail {
    dsn: string;
    dirs: string[];
}

interface SubmittedJob {
    jobname: string;
    jobid: string;
    details: JobDetail;
    downloaded: boolean;
    unpacking?: Promise<void>;
}

interface ParsedJobHeader {
    jobHeader: {
        prefix: string,
        replCount: number,
        suffix: string
    } | string;
    jobMask: string;
}
const translationTable: string = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
function prepareJobHeader(pattern: string): ParsedJobHeader {
    const match = /^(\/\/[^ ?]+)(\?*)( .*)$/.exec(pattern);

    if (!match)
        throw Error("Invalid JOB header");
    else if (match[2].length)
        return { jobHeader: { prefix: match[1], replCount: match[2].length, suffix: match[3] }, jobMask: match[1].slice(2) + '*' };
    else
        return { jobHeader: pattern, jobMask: pattern.slice(2, pattern.indexOf(' ')) };
}
function generateJobHeader(header: ParsedJobHeader, jobNo: number): string {
    if (typeof header.jobHeader === 'string')
        return header.jobHeader;
    else {
        let jobnameSuffix = jobNo.toString(36).toUpperCase();
        if (jobnameSuffix.length > header.jobHeader.replCount)
            jobnameSuffix = jobnameSuffix.slice(jobnameSuffix.length - header.jobHeader.replCount);
        else if (jobnameSuffix.length < header.jobHeader.replCount)
            jobnameSuffix = jobnameSuffix.padStart(header.jobHeader.replCount, '0');

        return header.jobHeader.prefix + jobnameSuffix + header.jobHeader.suffix;
    }
}

export function adjustJobHeader(hdr: string): string[] {
    const recordLength = 72;
    const splitFailed = () => {
        throw Error("Unable to split the job card into records: " + hdr);
    };

    hdr = hdr.trimEnd();
    if (hdr.length < recordLength) return [hdr];
    const parts = /^(\/\/\S+)\s+(\S+)(?:\s+(.*))?/.exec(hdr);
    if (!parts) splitFailed();
    if (parts.length < 4 || parts[3].length === 0) return [parts[1] + ' ' + parts[2]];
    const split_result = [];
    let to_split = parts[3];
    while (to_split.length > 0) {
        const symbol = to_split.search(/[,']/,);
        if (symbol === -1) {
            split_result.push(to_split);
            break;
        }
        switch (to_split.charAt(symbol)) {
            case ',':
                split_result.push(to_split.slice(0, symbol + 1));
                to_split = to_split.slice(symbol + 1);
                break;
            case '\'':
                const quoted = to_split.match(/[^']*'(?:[^']|'')*'\)*,?/);
                if (!quoted) splitFailed();
                split_result.push(quoted[0]);
                to_split = to_split.slice(quoted[0].length);
                break;
        }
    }
    const result = [parts[1] + ' ' + parts[2] + ' '];

    for (const x of split_result) {
        if (result[result.length - 1].length + x.length < recordLength)
            result[result.length - 1] += x;
        else
            result.push('// ' + x);
    }

    if (result.some(x => x.length >= recordLength)) splitFailed();
    return result;
}

function generateJcl(jobNo: number, jobcard: ParsedJobHeader, datasetName: string): string {
    return [
        ...adjustJobHeader(generateJobHeader(jobcard, jobNo)),
        "//AMATERSE EXEC PGM=AMATERSE,PARM=SPACK",
        "//SYSPRINT DD DUMMY",
        "//SYSIN    DD DUMMY",
        "//SYSUT1   DD DISP=SHR,DSN=" + datasetName,
        "//SYSUT2   DD DISP=(,PASS),DSN=&&TERSED,SPACE=(CYL,(10,10))",
        "//*",
        "//PRINTIT  EXEC PGM=IEBGENER",
        "//SYSPRINT DD DUMMY",
        "//SYSIN    DD DUMMY",
        "//SYSUT1   DD DISP=OLD,DSN=&&TERSED",
        "//SYSUT2   DD SYSOUT=*"
    ].join('\r\n')
}

function extractJobName(jcl: string): string {
    return jcl.slice(2, jcl.indexOf(' '));
}

async function submitJobs(client: JobClient, jobcard: ParsedJobHeader, jobList: JobDetail[], progress: StageProgressReporter, checkCancel: () => void): Promise<SubmittedJob[]> {
    let id = 0;

    let result: SubmittedJob[] = [];

    for (const e of jobList) {
        checkCancel();
        const jcl = generateJcl(id++, jobcard, e.dsn);
        const jobname = extractJobName(jcl);

        const jobid = await client.submitJcl(jcl);

        result.push({ jobname: jobname, jobid: jobid, details: e, downloaded: false });
        progress.stageCompleted();
    }
    return result;
}

function fixPath(p: string): string {
    p = p.replace(/\\/g, '/');
    while (p.endsWith('/')) // no lastIndexOfNot or trimEnd(x)?
        p = p.slice(0, p.length - 1);
    return p;
}

function getWasmRuntimeArgs(): Array<string> {
    const v8Version = process && process.versions && process.versions.v8 || "1.0";
    const v8Major = +v8Version.split(".")[0];
    if (v8Major >= 10)
        return [];
    else
        return [
            '--experimental-wasm-eh'
        ];
}

async function unterse(outDir: string): Promise<{ process: Promise<void>, input: Writable }> {
    await fsp.mkdir(outDir, { recursive: true });
    const unpacker = fork(
        path.join(__dirname, '..', 'bin', 'terse'),
        ["--op", "unpack", "--overwrite", "--copy-if-symlink-fails", "-o", outDir],
        { execArgv: getWasmRuntimeArgs(), stdio: ['pipe', 'ignore', 'pipe', 'ipc'] }
    );
    const promise = new Promise<void>((resolve, reject) => {
        unpacker.stderr!.on('data', (chunk) => console.log(chunk.toString()));
        unpacker.on('exit', (code, signal) => {
            if (code === 0)
                resolve();
            else if (code)
                reject("Unterse ended with error code: " + code);
            else
                reject("Signal received from unterse: " + signal);
        })
    });
    return { process: promise, input: unpacker.stdin! };
}

function toBufferArray(s: string): Buffer[] {
    if (s.length != 256)
        throw Error("Single byte conversion table expected");
    const result: Buffer[] = [];
    for (const c of s)
        result.push(Buffer.from(c));
    return result;
}

const ibm1148WithCrlfReplacement = toBufferArray('\u0000\u0001\u0002\u0003\u009C\u0009\u0086\u007F\u0097\u008D\u008E\u000B\u000C\ue00D\u000E\u000F\u0010\u0011\u0012\u0013\u009D\ue025\u0008\u0087\u0018\u0019\u0092\u008F\u001C\u001D\u001E\u001F\u0080\u0081\u0082\u0083\u0084\u000A\u0017\u001B\u0088\u0089\u008A\u008B\u008C\u0005\u0006\u0007\u0090\u0091\u0016\u0093\u0094\u0095\u0096\u0004\u0098\u0099\u009A\u009B\u0014\u0015\u009E\u001A\u0020\u00A0\u00E2\u00E4\u00E0\u00E1\u00E3\u00E5\u00E7\u00F1\u005B\u002E\u003C\u0028\u002B\u0021\u0026\u00E9\u00EA\u00EB\u00E8\u00ED\u00EE\u00EF\u00EC\u00DF\u005D\u0024\u002A\u0029\u003B\u005E\u002D\u002F\u00C2\u00C4\u00C0\u00C1\u00C3\u00C5\u00C7\u00D1\u00A6\u002C\u0025\u005F\u003E\u003F\u00F8\u00C9\u00CA\u00CB\u00C8\u00CD\u00CE\u00CF\u00CC\u0060\u003A\u0023\u0040\u0027\u003D\u0022\u00D8\u0061\u0062\u0063\u0064\u0065\u0066\u0067\u0068\u0069\u00AB\u00BB\u00F0\u00FD\u00FE\u00B1\u00B0\u006A\u006B\u006C\u006D\u006E\u006F\u0070\u0071\u0072\u00AA\u00BA\u00E6\u00B8\u00C6\u20AC\u00B5\u007E\u0073\u0074\u0075\u0076\u0077\u0078\u0079\u007A\u00A1\u00BF\u00D0\u00DD\u00DE\u00AE\u00A2\u00A3\u00A5\u00B7\u00A9\u00A7\u00B6\u00BC\u00BD\u00BE\u00AC\u007C\u00AF\u00A8\u00B4\u00D7\u007B\u0041\u0042\u0043\u0044\u0045\u0046\u0047\u0048\u0049\u00AD\u00F4\u00F6\u00F2\u00F3\u00F5\u007D\u004A\u004B\u004C\u004D\u004E\u004F\u0050\u0051\u0052\u00B9\u00FB\u00FC\u00F9\u00FA\u00FF\u005C\u00F7\u0053\u0054\u0055\u0056\u0057\u0058\u0059\u005A\u00B2\u00D4\u00D6\u00D2\u00D3\u00D5\u0030\u0031\u0032\u0033\u0034\u0035\u0036\u0037\u0038\u0039\u00B3\u00DB\u00DC\u00D9\u00DA\u009F');

export function convertBuffer(buffer: Buffer, lrecl: number) {
    const EOLBuffer = Buffer.from(EOL);
    // 0xe000 private plane has 3 byte encoding sequence
    const result = Buffer.allocUnsafe(3 * buffer.length + Math.floor((buffer.length + lrecl - 1) / lrecl) * EOLBuffer.length);
    let pos = 0;
    let i = 0;
    for (const v of buffer) {
        pos += ibm1148WithCrlfReplacement[v].copy(result, pos, 0);
        if (i % lrecl === lrecl - 1)
            pos += EOLBuffer.copy(result, pos);
        ++i;
    }
    return result.subarray(0, pos);
}

async function translateFiles(dir: string) {
    const files = await fsp.readdir(dir, { withFileTypes: true });
    for (const file of files) {
        if (!file.isFile() || file.isSymbolicLink())
            continue;
        const filePath = path.join(dir, file.name);
        await fsp.writeFile(filePath, convertBuffer(await fsp.readFile(filePath), 80), "utf-8");
    }
}

async function copyDirectory(source: string, target: string) {
    await fsp.mkdir(target, { recursive: true });
    const files = await fsp.readdir(source, { withFileTypes: true });
    for (const file of files) {
        if (!file.isFile() || file.isSymbolicLink())
            continue;
        await fsp.copyFile(path.join(source, file.name), path.join(target, file.name));
    }
    for (const file of files) {
        if (!file.isSymbolicLink())
            continue;
        await fsp.symlink(await fsp.readlink(path.join(source, file.name)), path.join(target, file.name));
    }
}

export interface IoOps {
    unterse: (outDir: string) => Promise<{ process: Promise<void>, input: Writable }>;
    translateFiles: (dir: string) => Promise<void>;
    copyDirectory: (source: string, target: string) => Promise<void>;
}

async function downloadJobAndProcess(
    client: JobClient,
    fileInfo: JobDescription,
    job: SubmittedJob,
    progress: StageProgressReporter,
    io: IoOps): Promise<{ unpacker: Promise<void> }> {
    const jobDetail = getJobDetailInfo(fileInfo);
    if (!jobDetail || jobDetail.rc !== 0) {
        job.downloaded = true; // nothing we can do ...
        return {
            unpacker: Promise.reject(Error("Job failed: " + job.jobname + "/" + job.jobid))
        };
    }

    const firstDir = fixPath(job.details.dirs[0]);

    try {
        const { process, input } = await io.unterse(firstDir);
        await client.download(input, job.jobid, jobDetail.spoolFiles!);
        progress.stageCompleted();
        job.downloaded = true;

        return {
            unpacker:
                (async () => {
                    await process;
                    progress.stageCompleted();

                    await io.translateFiles(firstDir);

                    progress.stageCompleted();

                    for (const dir__ of job.details.dirs.slice(1)) {
                        await io.copyDirectory(firstDir, fixPath(dir__));
                        progress.stageCompleted();
                    }
                })()
        };
    }
    catch (e) {
        return {
            unpacker: Promise.reject(e)
        };
    }
}

export async function downloadDependenciesWithClient(client: JobClient,
    jobList: JobDetail[],
    jobcardPattern: string,
    progress: StageProgressReporter,
    io: IoOps,
    cancelled: () => boolean): Promise<{ failed: JobDetail[]; total: number; }> {

    const checkCancel = () => {
        if (cancelled())
            throw Error(cancelMessage);
    };

    try {
        const jobcard = prepareJobHeader(jobcardPattern);
        const jobs = await submitJobs(client, jobcard, jobList, progress, checkCancel);
        const jobsMap = jobs.reduce((result: { [key: string]: SubmittedJob }, x) => { result[x.jobname + "." + x.jobid] = x; return result; }, {});

        await client.setListMask(jobcard.jobMask);

        let wait = 0;
        let result = { failed: new Array<JobDetail>(), total: 0 };
        while (jobs.some(x => !x.downloaded)) {
            checkCancel();

            const list = (await client.list()).map(x => {
                const j = jobsMap[x.jobname + "." + x.id];
                return { fileInfo: x, job: j && !j.downloaded ? j : null };
            }).filter(x => !!x.job);

            for (const l of list) {
                const job = l.job!;
                job.unpacking = (await downloadJobAndProcess(client, l.fileInfo, job, progress, io)).unpacker
                    .then(_ => { result.total++; })
                    .catch(_ => { result.total++; result.failed.push(job.details); });
            }

            if (list.length === 0) {
                if (wait < 30)
                    wait += 1;
                await new Promise((resolve) => setTimeout(resolve, wait * 1000))
            }
            else
                wait = 0;
        }

        await Promise.all(jobs.map(x => x.unpacking));

        return result;
    }
    finally {
        client.dispose();
    }
}

function askUser(prompt: string, password: boolean, defaultValue: string = ''): Promise<string> {
    const input = vscode.window.createInputBox();
    return new Promise<string>((resolve, reject) => {
        input.ignoreFocusOut = true;
        input.prompt = prompt;
        input.password = password;
        input.value = defaultValue || '';
        input.onDidHide(() => reject(Error(cancelMessage)));
        input.onDidAccept(() => resolve(input.value));
        input.show();
    }).finally(() => { input.dispose(); });
}

function pickUser<T>(title: string, options: { label: string, value: T }[]): Promise<T> {
    const input = vscode.window.createQuickPick();
    return new Promise<T>((resolve, reject) => {
        input.ignoreFocusOut = true;
        input.title = title;
        input.items = options.map(x => { return { label: x.label }; });
        input.canSelectMany = false;
        input.onDidHide(() => reject(Error(cancelMessage)));
        input.onDidAccept(() => resolve(options.find(x => x.label === input.selectedItems[0].label)!.value));
        input.show();
    }).finally(() => { input.dispose(); });
}

// because VSCode does not expose the service???
export function replaceVariables(obj: any, resolver: (configKey: string) => (string | undefined), worksapceUri: vscode.Uri): any {
    if (typeof obj === 'object') {
        for (const x in obj)
            obj[x] = replaceVariables(obj[x], resolver, worksapceUri);
    }
    else if (typeof obj === 'string') {
        while (true) {
            const match = /\$\{config:([^}]+)\}|\$\{(workspaceFolder)\}/.exec(obj);
            if (!match) break;

            const replacement = match[1] ? '' + resolver(match[1]) : worksapceUri;
            obj = obj.slice(0, match.index) + replacement + obj.slice(match.index + match[0].length);
        }
    }
    return obj;
}

async function gatherAvailableConfigs() {
    if (vscode.workspace.workspaceFolders === undefined) return [];
    const availableConfigs = (await Promise.all(vscode.workspace.workspaceFolders.map(x => {
        return new Promise<{ workspace: vscode.WorkspaceFolder, config: any } | null>((resolve) => {
            vscode.workspace.openTextDocument(vscode.Uri.joinPath(x.uri, hlasmplugin_folder, proc_grps_file)).then((doc) => resolve({ workspace: x, config: JSON.parse(doc.getText()) }), _ => resolve(null))
        })
    }))).filter(x => !!x).map(x => x!);

    const varResolver = (workspace: vscode.WorkspaceFolder) => {
        const config = vscode.workspace.getConfiguration(undefined, workspace);
        return (s: string) => config.get<string>(s);
    }

    return availableConfigs.map(x => { return { workspaceUri: x.workspace.uri, config: replaceVariables(x.config, varResolver(x.workspace), x.workspace.uri) } });
}

export function extractDsn(d: string | undefined, workspaceUri: vscode.Uri): { dsn: string, path: string } | null {
    const guessDsnRegex = /(?:.*[\\/])?((?:[A-Za-z0-9@#$]{1,8})(?:\.[A-Za-z0-9@#$]{1,8})+)[\\/]*/;

    if (!d || d.length === 0)
        return null;

    const dsn_match = guessDsnRegex.exec(d);
    if (dsn_match) {
        const dsn = dsn_match[1].toUpperCase();
        if (d.startsWith("~"))
            return { dsn: dsn, path: fixPath(path.join(homedir(), /~[\\/]/.test(d) ? d.slice(2) : d.slice(1))) };
        else if (/^[A-Za-z][A-Za-z0-9+.-]+:/.test(d)) { // url (and not windows path)
            const uri = vscode.Uri.parse(d);
            if (uri.scheme === 'file')
                return { dsn: dsn, path: fixPath(uri.fsPath) };
        }
        else { // path
            const uri = path.isAbsolute(d) ? vscode.Uri.file(d) : vscode.Uri.joinPath(workspaceUri, d);
            if (uri.scheme === 'file')
                return { dsn: dsn, path: fixPath(uri.fsPath) };
        }
    }
    return null;
}

export function gatherDownloadList(availableConfigs: { workspaceUri: vscode.Uri, config: any }[]) {
    const collectedDsnAndPath: { [key: string]: string[] } = {};

    for (const c of availableConfigs) {
        for (const pg of c.config.pgroups) {
            for (const l of pg.libs) {
                const dsn = extractDsn(typeof l === 'string' && l || typeof l.path === 'string' && l.path, c.workspaceUri);
                if (dsn) {
                    if (dsn.dsn in collectedDsnAndPath)
                        collectedDsnAndPath[dsn.dsn].push(dsn.path);
                    else
                        collectedDsnAndPath[dsn.dsn] = [dsn.path];
                }
            }
        }
    }

    const thingsToDownload: JobDetail[] = [];
    for (const key in collectedDsnAndPath)
        thingsToDownload.push({ dsn: key, dirs: [... new Set<string>(collectedDsnAndPath[key])] });

    return thingsToDownload;
}

async function filterDownloadList(downloadCandidates: JobDetail[], newOnly: boolean): Promise<JobDetail[]> {
    const input = vscode.window.createQuickPick();

    const interestingDsn = new Set<string>();
    if (newOnly) {
        for (const job of downloadCandidates) {
            for (const dir of job.dirs) {
                if (await isDirectoryEmpty(dir)) {
                    interestingDsn.add(job.dsn);
                    break;
                }
            };
        }
    }
    return new Promise<JobDetail[]>(async (resolve, reject) => {
        input.ignoreFocusOut = true;
        input.title = 'Select data sets to download';
        input.items = downloadCandidates.map(x => { return { label: x.dsn }; });
        input.canSelectMany = true;
        input.selectedItems = newOnly ? input.items.filter(x => interestingDsn.has(x.label)) : input.items;
        input.onDidHide(() => reject(Error(cancelMessage)));
        input.onDidAccept(() => {
            const selected = new Set(input.selectedItems.map(x => x.label));
            resolve(downloadCandidates.filter(x => selected.has(x.dsn)));
        });
        input.show();
    }).finally(() => { input.dispose(); });
}

enum connectionSecurityLevel {
    "rejectUnauthorized",
    "acceptUnauthorized",
    "unsecure",
}

interface ConnectionInfo {
    host: string;
    port: number | undefined;
    user: string;
    password: string;
    hostInput: string;
    securityLevel: connectionSecurityLevel;

    zowe: boolean;
}

function gatherSecurityLevelFromZowe(profile: any) {
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
    await zoweExplorerApi
        .getExplorerExtenderApi()
        .getProfilesCache()
        .refresh(zoweExplorerApi);
    const loadedProfile = zoweExplorerApi
        .getExplorerExtenderApi()
        .getProfilesCache()
        .loadNamedProfile(profileName);

    return {
        host: loadedProfile.profile.host,
        port: loadedProfile.profile.port,
        user: loadedProfile.profile.user,
        password: loadedProfile.profile.password,
        hostInput: '@' + profileName,
        securityLevel: gatherSecurityLevelFromZowe(loadedProfile.profile),
        zowe: true,
    };
}

async function gatherConnectionInfo(lastInput: DownloadDependenciesInputMemento): Promise<ConnectionInfo> {
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
    const securityLevel = await pickUser("Select security option", [
        { label: "Use TLS, reject unauthorized certificated", value: connectionSecurityLevel.rejectUnauthorized },
        { label: "Use TLS, accept unauthorized certificated", value: connectionSecurityLevel.acceptUnauthorized },
        { label: "Unsecured connection", value: connectionSecurityLevel.unsecure },
    ]);
    return { host, port, user, password, hostInput, securityLevel, zowe: false };
}

interface DownloadDependenciesInputMemento {
    host: string;
    user: string;
    jobcard: string;
}

const mementoKey = "hlasm.downloadDependencies";

function getLastRunConfig(context: vscode.ExtensionContext): DownloadDependenciesInputMemento {
    let lastRun = context.globalState.get(mementoKey, { host: '', user: '', jobcard: '' });
    return {
        host: '' + (lastRun.host || ''),
        user: '' + (lastRun.user || ''),
        jobcard: '' + (lastRun.jobcard || ''),
    };
}

async function isDirectoryEmpty(dir: string): Promise<boolean> {
    try {
        const dirIter = await fsp.opendir(dir);
        const { value, done } = await dirIter[Symbol.asyncIterator]().next();
        if (!done) {
            await dirIter.close(); // async iterator is closed automatically when the last entry is produced
            return false;
        }
    }
    catch (e) { if (e.code !== 'ENOENT') throw e; }

    return true;
}

async function checkForExistingFiles(jobs: JobDetail[]) {
    const uniqueDirs = new Set<string>();
    jobs.forEach(x => x.dirs.forEach(y => uniqueDirs.add(y)));

    const nonemptyDirs = new Set<string>();

    for (const d of uniqueDirs)
        if (!await isDirectoryEmpty(d))
            nonemptyDirs.add(d);

    return nonemptyDirs;
}

async function removeFilesFromDirectory(dir: string) {
    const files = await fsp.readdir(dir, { withFileTypes: true });
    for (const f of files) {
        if (f.isFile() || f.isSymbolicLink())
            await fsp.unlink(path.join(dir, f.name));
    }
}

export interface StageProgressReporter {
    stageCompleted(): void;
}

class ProgressReporter implements StageProgressReporter {
    constructor(private p: vscode.Progress<{ message?: string; increment?: number }>, private stages: number) { }
    stageCompleted(): void {
        this.p.report({ increment: 100 / this.stages });
    }
}

export async function downloadDependencies(context: vscode.ExtensionContext, telemetry: Telemetry, ...args: any[]) {
    try {
        telemetry.reportEvent("downloadDependencies/started");

        const newOnly = args.length === 1 && args[0] === "newOnly";
        const lastInput = getLastRunConfig(context);
        const { host, port, user, password, hostInput, securityLevel, zowe } = await gatherConnectionInfo(lastInput);

        const jobcardPattern = await askUser("Enter jobcard pattern (? will be substituted)", false, lastInput.jobcard || "//" + user.slice(0, 7).padEnd(8, '?').toUpperCase() + " JOB ACCTNO");

        await context.globalState.update(mementoKey, { host: hostInput, user: user, jobcard: jobcardPattern });

        const thingsToDownload = await filterDownloadList(gatherDownloadList(await gatherAvailableConfigs()), newOnly);

        const dirsWithFiles = await checkForExistingFiles(thingsToDownload);
        if (dirsWithFiles.size > 0) {
            const overwrite = "Overwrite";
            const whatToDo = await vscode.window.showQuickPick([overwrite, "Cancel"], { title: "Some of the directories (" + dirsWithFiles.size + ") exist and are not empty." });
            if (whatToDo !== overwrite)
                return;

            for (const d of dirsWithFiles)
                await removeFilesFromDirectory(d);
        }

        const startTime = Date.now();

        const result = await vscode.window.withProgress({ title: "Downloading dependencies", location: vscode.ProgressLocation.Notification, cancellable: true }, async (p, t) => {
            return downloadDependenciesWithClient(
                await basicFtpJobClient({
                    host: host,
                    user: user,
                    password: password,
                    port: port,
                    securityLevel: securityLevel
                }),
                thingsToDownload,
                jobcardPattern,
                new ProgressReporter(p, thingsToDownload.reduce((prev, cur) => { return prev + cur.dirs.length + 3 }, 0)),
                { unterse, translateFiles, copyDirectory },
                () => t.isCancellationRequested);
        });

        const endTime = Date.now();


        const showFailedJobs = "Show failed jobs";
        if (result.failed.length > 0) // TODO: offer re-run?
            vscode.window.showErrorMessage(result.failed.length + " jobs out of " + result.total + " failed", showFailedJobs).then((choice) => {
                if (choice !== showFailedJobs) return;
                const channel: vscode.OutputChannel = context.extension.exports.getExtension().outputChannel;
                channel.appendLine("The following data sets could not have been downloaded:");
                result.failed.forEach(x => { channel.append('  '); channel.appendLine(x.dsn) });
                channel.show();
            });
        else
            vscode.window.showInformationMessage("All jobs (" + result.total + ") completed successfully");

        telemetry.reportEvent("downloadDependencies/finished", { zowe: zowe ? 'yes' : 'no' }, { failed: result.failed.length, total: result.total, elapsedTime: (endTime - startTime) / 1000 });
    }
    catch (e) {
        if (e.message !== cancelMessage)
            vscode.window.showErrorMessage("Error occured while downloading dependencies: " + (e.message || e));
    }
}
