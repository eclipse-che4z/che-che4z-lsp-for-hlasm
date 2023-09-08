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
import { Client, FTPResponse, FileInfo, FTPError } from 'basic-ftp'
import { Readable, Writable } from 'stream';
import { homedir } from 'os';
import * as path from 'path';
import { promises as fsp } from "fs";
import { hlasmplugin_folder, proc_grps_file } from './constants';
import { Telemetry } from './telemetry';
import { askUser } from './uiUtils';
import { connectionSecurityLevel, gatherConnectionInfo, getLastRunConfig, translateConnectionInfo, updateLastRunConfig } from './ftpCreds';
import { isCancellationError } from './helpers';
import { unterseFile } from "terse.js";
import { FBStreamingConvertor } from './FBStreamingConvertor';

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
    await client.access(translateConnectionInfo(connection));

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
    const splitFailed: () => never = () => {
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
                const quoted = /[^']*'(?:[^']|'')*'\)*,?/.exec(to_split);
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

export async function unterse(outDir: string): Promise<{ process: Promise<void>, input: Writable }> {
    await fsp.mkdir(outDir, { recursive: true });

    class DownloadStream extends Writable {
        private chunks: Uint8Array[] = [];
        private done = false;
        private notinterested = false;
        private pendingFetch?: { resolve: () => void, reject: (e: Error) => void };

        _write(chunk: Buffer, encoding: BufferEncoding, callback: (error?: Error | null) => void) {
            if (this.notinterested) return;
            this.chunks.push(chunk);
            this.getCallback()?.resolve();

            callback();
        }

        _final(callback: (error?: Error | null) => void) {
            this.done = true;
            this.getCallback()?.reject(Error("Unexpected EOF"));
            callback();
        }

        private getCallback() {
            const cb = this.pendingFetch;
            this.pendingFetch = undefined;
            return cb;
        }

        async fetchChunk() {
            while (true) {
                if (this.chunks.length > 0)
                    return this.chunks.shift()!;
                else if (this.done)
                    throw Error("Unexpected EOF");

                await new Promise<void>((resolve, reject) => { input.pendingFetch = { resolve, reject } });
            }
        }

        lostInterest() { this.notinterested = true; }
    };
    const input = new DownloadStream();

    let pendingTake: number[] = [];
    let pendingAction = Promise.resolve();
    let activeActions = 0;
    let fb: FBStreamingConvertor | undefined;
    let currentMember = '';

    const scheduleAction = (action: () => Promise<void>) => {
        ++activeActions;
        pendingAction = pendingAction.then(action).finally(() => --activeActions);
    };

    const writeCurrentMember = () => {
        if (!fb) return;

        const _currentMember = currentMember;
        const _fb = fb;

        scheduleAction(() => fsp.writeFile(path.join(outDir, _currentMember), _fb.getResult()));

        fb = undefined;
        currentMember = '';
    };

    const { resolve, error, process } = (() => {
        let _resolve: (() => void) | undefined;
        let _error: ((e: Error) => void) | undefined;
        const process = new Promise<void>((r, e) => { _resolve = r; _error = e; });
        return { resolve: _resolve!, error: _error!, process };
    })();

    unterseFile({
        async take(n: number): Promise<number[]> {
            while (pendingTake.length < n)
                pendingTake = pendingTake.concat(...await input.fetchChunk());
            return pendingTake.splice(0, n);
        },
        async rest(f: (b: Uint8Array) => boolean | Promise<boolean>): Promise<void> {
            if (pendingTake.length > 0) {
                const b = Uint8Array.from(pendingTake);
                pendingTake = [];
                if (!await f(b))
                    return;
            }
            while (await f(await input.fetchChunk())) {
                if (activeActions > 16)
                    await pendingAction;
            }
            await pendingAction;
        },
    }, (header) => {
        if (!header.pds) throw Error("PDS(E) expected");
        if (header.lrecl !== 80 || !header.recfm.startsWith('F')) throw Error("Expected FB80 data set");

        return {
            write: (data: Uint8Array) => {
                fb!.write(data)
            },
            check_next_member: (name: string) => {
                if (!/[A-Z$#@][A-Z$#@0-9]*/.test(name))
                    throw Error("Invalid member name");
            },
            start_member: (name: string) => {
                writeCurrentMember();

                fb = new FBStreamingConvertor(80);
                currentMember = name;
            },
            alias_member: (name: string, target: string) => {
                writeCurrentMember();

                scheduleAction(() => fsp.symlink(path.join(outDir, target), path.join(outDir, name)).catch(
                    () => fsp.copyFile(path.join(outDir, target), path.join(outDir, name))
                ));
            },
        };
    }).then(() => {
        input.lostInterest();

        writeCurrentMember();

        return pendingAction;
    }).then(resolve).catch(error);
    return { process, input };
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
            throw new vscode.CancellationError();
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
    return new Promise<JobDetail[]>((resolve, reject) => {
        input.ignoreFocusOut = true;
        input.title = 'Select data sets to download';
        input.items = downloadCandidates.map(x => { return { label: x.dsn }; });
        input.canSelectMany = true;
        input.selectedItems = newOnly ? input.items.filter(x => interestingDsn.has(x.label)) : input.items;
        input.onDidHide(() => reject(new vscode.CancellationError()));
        input.onDidAccept(() => {
            const selected = new Set(input.selectedItems.map(x => x.label));
            resolve(downloadCandidates.filter(x => selected.has(x.dsn)));
        });
        input.show();
    }).finally(() => { input.dispose(); });
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
    catch (e) { if (e instanceof Error && 'code' in e && e.code !== 'ENOENT') throw e; }

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

export async function downloadDependencies(context: vscode.ExtensionContext, telemetry: Telemetry, channel: vscode.OutputChannel, ...args: any[]) {
    try {
        telemetry.reportEvent("downloadDependencies/started");

        const newOnly = args.length === 1 && args[0] === "newOnly";
        const lastInput = getLastRunConfig(context);
        const { host, port, user, password, hostInput, securityLevel, zowe } = await gatherConnectionInfo(lastInput);

        const jobcardPattern = await askUser("Enter jobcard pattern (? will be substituted)", false, lastInput.jobcard || "//" + user.slice(0, 7).padEnd(8, '?').toUpperCase() + " JOB ACCTNO");

        await updateLastRunConfig(context, { host: hostInput, user: user, jobcard: jobcardPattern });

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
                new ProgressReporter(p, thingsToDownload.reduce((prev, cur) => { return prev + cur.dirs.length + 2 }, 0)),
                { unterse, copyDirectory },
                () => t.isCancellationRequested);
        });

        const endTime = Date.now();


        const showFailedJobs = "Show failed jobs";
        if (result.failed.length > 0) // TODO: offer re-run?
            vscode.window.showErrorMessage(result.failed.length + " jobs out of " + result.total + " failed", showFailedJobs).then((choice) => {
                if (choice !== showFailedJobs) return;
                channel.appendLine("The following data sets could not have been downloaded:");
                result.failed.forEach(x => { channel.append('  '); channel.appendLine(x.dsn) });
                channel.show();
            });
        else
            vscode.window.showInformationMessage("All jobs (" + result.total + ") completed successfully");

        telemetry.reportEvent("downloadDependencies/finished", { zowe: zowe ? 'yes' : 'no' }, { failed: result.failed.length, total: result.total, elapsedTime: (endTime - startTime) / 1000 });
    }
    catch (e) {
        if (isCancellationError(e))
            return;
        vscode.window.showErrorMessage("Error occured while downloading dependencies: " + (e instanceof Error && e.message || e));
    }
}
