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

import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient';


import { HLASMConfigurationProvider, getCurrentProgramName, getProgramName } from './debugProvider';
import { insertContinuation, removeContinuation, rearrangeSequenceNumbers } from './continuationHandler';
import { CustomEditorCommands } from './customEditorCommands';
import { EventsHandler, getConfig } from './eventsHandler';
import { ServerVariant } from './serverFactory.common';
import { createLanguageServer } from './serverFactory';
import { HLASMDebugAdapterFactory } from './hlasmDebugAdapterFactory';
import { Telemetry, createTelemetry } from './telemetry';
import { LanguageClientErrorHandler } from './languageClientErrorHandler';
import { HLASMVirtualFileContentProvider } from './hlasmVirtualFileContentProvider';
import { downloadDependencies } from './hlasmDownloadCommands';
import { blockCommentCommand, CommentOption, lineCommentCommand } from './commentEditorCommands';
import { HLASMCodeActionsProvider } from './hlasmCodeActionsProvider';
import { hlasmplugin_folder_filter, bridge_json_filter, schemeExternalFiles, continuationColumn, initialBlanks, languageIdHlasm, debugTypeHlasm, schemeVirtualFiles } from './constants';
import { ConfigurationsHandler } from './configurationsHandler';
import { HlasmPluginMiddleware, getLanguageClientMiddleware } from './languageClientMiddleware';
import { HLASMExternalFiles } from './hlasmExternalFiles';
import { HLASMExternalFilesFtp } from './hlasmExternalFilesFtp';
import { HLASMExternalConfigurationProvider, HLASMExternalConfigurationProviderHandler } from './hlasmExternalConfigurationProvider';
import { HlasmExtension } from './extension.interface';
import { toggleAdvisoryConfigurationDiagnostics } from './hlasmConfigurationDiagnosticsProvider'
import { handleE4EIntegration } from './hlasmExternalFilesEndevor';
import { pickUser } from './uiUtils';
import { activateBranchDecorator } from './branchDecorator';
import { asError } from './helpers';
import { registerListingServices } from './hlasmListingServices';
import { MementoKey } from './mementoKeys';
import { registerOutputDocumentContentProvider, showOutputCommand } from './hlasmOutputContentProvider';

const sleep = (ms: number) => {
    return new Promise((resolve) => { setTimeout(resolve, ms) });
};

function objectToString(o: any) {
    if (o === null)
        return null;

    Object.keys(o).forEach(k => {
        o[k] = '' + o[k];
    });

    return o;
}

const getCacheInfo = async (uri: vscode.Uri, fs: vscode.FileSystem) => {
    try {
        await fs.createDirectory(uri);
        return { uri, fs };
    }
    catch (e) {
        vscode.window.showErrorMessage('Unable to create cache directory for external resources', ...(e instanceof Error ? [e.message] : []));
        return undefined;
    }
}

function whenString(x: any): string | undefined {
    if (typeof x === 'string')
        return x;
    else
        return undefined;
}

/**
 * ACTIVATION
 * activates the extension
 */
export async function activate(context: vscode.ExtensionContext): Promise<HlasmExtension> {
    const serverVariant = getConfig<ServerVariant>('serverVariant', 'native');
    const version = whenString(context.extension.packageJSON?.version);

    const telemetry = createTelemetry();
    context.subscriptions.push(telemetry);

    telemetry.reportEvent('hlasm.activated', {
        server_variant: serverVariant.toString(),
        showBranchInformation: getConfig<boolean>('showBranchInformation', true).toString(),
    });

    registerListingServices(context);
    await registerEditHelpers(context);

    const clientErrorHandler = new LanguageClientErrorHandler(telemetry);
    const middleware = getLanguageClientMiddleware();
    // create client options
    const clientOptions: vscodelc.LanguageClientOptions = {
        documentSelector: [{ language: languageIdHlasm }],
        errorHandler: clientErrorHandler,
        middleware: middleware,
    };

    const extConfProvider = new HLASMExternalConfigurationProvider();
    const extFiles = new HLASMExternalFiles(
        schemeExternalFiles,
        vscode.workspace.fs,
        await getCacheInfo(vscode.Uri.joinPath(context.globalStorageUri, 'external.files.cache'), vscode.workspace.fs)
    );
    context.subscriptions.push(extFiles);

    const hlasmpluginClient = await startLanguageServerWithFallback({
        version, serverVariant, clientOptions, context, telemetry, clientErrorHandler, middleware, extConfProvider, extFiles,
    });

    // register all commands and objects to context
    await registerDebugSupport(context, hlasmpluginClient);
    await registerExternalFileSupport(context, hlasmpluginClient, extFiles);
    await registerToContextWithClient(context, hlasmpluginClient, telemetry);
    registerOutputDocumentContentProvider(hlasmpluginClient, context.subscriptions);

    let api: HlasmExtension = {
        registerExternalFileClient(service, client) {
            return extFiles.setClient(service, client);
        },
        registerExternalConfigurationProvider(h: HLASMExternalConfigurationProviderHandler) {
            return extConfProvider.addHandler(h);
        },
    };

    handleE4EIntegration(api, context.subscriptions, hlasmpluginClient.outputChannel);

    return api;
}

type LangStartOptions = {
    version: string | undefined,
    serverVariant: ServerVariant;
    clientOptions: vscodelc.LanguageClientOptions;
    context: vscode.ExtensionContext;
    telemetry: Telemetry,
    clientErrorHandler: LanguageClientErrorHandler,
    middleware: HlasmPluginMiddleware,
    extConfProvider: HLASMExternalConfigurationProvider,
    extFiles: HLASMExternalFiles,
}

async function startLanguageServerWithFallback(opts: LangStartOptions) {
    let lsResult = await startLanguageServer(opts);
    if (!(lsResult instanceof Error))
        return lsResult;

    if (opts.serverVariant === 'wasm') {
        opts.telemetry.reportException(lsResult);
        throw lsResult;
    }

    const lastCrashVersion = opts.context.globalState.get(MementoKey.LastCrashVersion);
    if (opts.version)
        await opts.context.globalState.update(MementoKey.LastCrashVersion, opts.version);

    if (!opts.version || opts.version !== lastCrashVersion)
        vscode.window.showWarningMessage('The language server did not start. Switching to WASM version.');

    opts.telemetry.reportEvent('hlasm.wasmFallback');

    opts.serverVariant = 'wasm';
    lsResult = await startLanguageServer(opts);

    if (!(lsResult instanceof Error))
        return lsResult;

    opts.telemetry.reportException(lsResult);
    throw lsResult;
}

async function startLanguageServer(opts: LangStartOptions): Promise<vscodelc.BaseLanguageClient | Error> {
    const disposables: vscode.Disposable[] = [];

    //client init
    const hlasmpluginClient = await createLanguageServer(opts.serverVariant, opts.clientOptions, opts.context.extensionUri);

    disposables.push(hlasmpluginClient.onDidChangeState(e => e.newState === vscodelc.State.Starting && opts.middleware.resetFirstOpen()));

    opts.clientErrorHandler.defaultHandler = hlasmpluginClient.createDefaultErrorHandler();

    // The objectToString is necessary, because telemetry reporter only takes objects with
    // string properties and there are some boolean that we receive from the language server
    disposables.push(hlasmpluginClient.onTelemetry((object) => { opts.telemetry.reportEvent(object.method_name, objectToString(object.properties), object.measurements) }));

    disposables.push(opts.extConfProvider.attach(hlasmpluginClient));
    disposables.push(opts.extFiles.attach(hlasmpluginClient));

    //give the server some time to start listening when using TCP
    if (opts.serverVariant === 'tcp')
        await sleep(2000);

    try {
        await hlasmpluginClient.start();
        opts.context.subscriptions.push(hlasmpluginClient, ...disposables);
        return hlasmpluginClient;
    }
    catch (e) {
        const err = asError(e);
        disposables.reverse().forEach(x => x.dispose());
        opts.clientErrorHandler.defaultHandler = undefined;
        return err;
    }
}

async function registerEditHelpers(context: vscode.ExtensionContext) {
    const completeCommand = "editor.action.triggerSuggest";

    // initialize helpers
    const handler = new EventsHandler(completeCommand);
    const commands = new CustomEditorCommands();

    // register them
    context.subscriptions.push(commands);
    context.subscriptions.push(handler);

    // overrides should happen only if the user wishes
    if (getConfig<boolean>('continuationHandling', false)) {
        try {
            context.subscriptions.push(vscode.commands.registerTextEditorCommand("type",
                (editor, edit, args) => commands.insertChars(editor, edit, args, continuationColumn)));
            context.subscriptions.push(vscode.commands.registerTextEditorCommand("paste",
                (editor, edit, args) => commands.insertChars(editor, edit, args, continuationColumn)));
            context.subscriptions.push(vscode.commands.registerTextEditorCommand("cut",
                (editor, edit) => commands.cut(editor, edit, continuationColumn)));
            context.subscriptions.push(vscode.commands.registerTextEditorCommand("deleteLeft",
                (editor, edit) => commands.deleteLeft(editor, edit, continuationColumn)));
            context.subscriptions.push(vscode.commands.registerTextEditorCommand("deleteRight",
                (editor, edit) => commands.deleteRight(editor, edit, continuationColumn)));
        } catch (e) { /* just ignore */ }
    }
    // register event handlers
    context.subscriptions.push(vscode.workspace.onDidChangeTextDocument(e => handler.onDidChangeTextDocument(e, continuationColumn)));
    context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(e => handler.onDidOpenTextDocument(e)));
    context.subscriptions.push(vscode.workspace.onDidChangeConfiguration(e => handler.onDidChangeConfiguration(e)));
    context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(e => handler.onDidSaveTextDocument(e)));
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(e => handler.onDidChangeActiveTextEditor(e)));
    context.subscriptions.push(vscode.workspace.onDidChangeWorkspaceFolders(e => handler.onDidChangeWorkspaceFolders(e)));

    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.createCompleteConfig', ConfigurationsHandler.createCompleteConfig));

    context.subscriptions.push(vscode.languages.registerCodeLensProvider({ language: languageIdHlasm }, { provideCodeLenses: ConfigurationsHandler.provideCodeLenses }));

    // register continuation handlers
    if (!((await vscode.commands.getCommands()).find(command => command == "extension.hlasm-plugin.insertContinuation" || command == "extension.hlasm-plugin.removeContinuation"))) {
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.insertContinuation",
            (editor, edit) => insertContinuation(editor, edit, continuationColumn, initialBlanks)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.removeContinuation",
            (editor, edit) => removeContinuation(editor, edit, continuationColumn)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.rearrangeSequenceNumbers",
            (editor, edit) => rearrangeSequenceNumbers(editor, edit, continuationColumn)));

        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.toggleCommentEditorCommands",
            (editor, edit) => lineCommentCommand(editor, edit, CommentOption.toggle)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.addCommentEditorCommands",
            (editor, edit) => lineCommentCommand(editor, edit, CommentOption.add)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.removeCommentEditorCommands",
            (editor, edit) => lineCommentCommand(editor, edit, CommentOption.remove)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.blockCommentEditorCommands",
            (editor, edit) => blockCommentCommand(editor, edit)));
    }
}
async function registerDebugSupport(context: vscode.ExtensionContext, client: vscodelc.BaseLanguageClient) {
    // register filename retrieve functions for debug sessions
    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.getProgramName', () => getProgramName()));
    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.getCurrentProgramName', () => getCurrentProgramName()));

    // register provider for all hlasm debug configurations
    context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider(debugTypeHlasm, new HLASMConfigurationProvider()));
    context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory(debugTypeHlasm, new HLASMDebugAdapterFactory(client)));
}

async function registerExternalFileSupport(context: vscode.ExtensionContext, client: vscodelc.BaseLanguageClient, extFiles: HLASMExternalFiles) {
    context.subscriptions.push(client.onDidChangeState(e => e.newState === vscodelc.State.Starting && extFiles.reset()));
    context.subscriptions.push(vscode.workspace.registerTextDocumentContentProvider(schemeExternalFiles, extFiles.getTextDocumentContentProvider()));
    const datasetClient = HLASMExternalFilesFtp(context);
    if (datasetClient)
        extFiles.setClient('DATASET', datasetClient);

    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.resumeRemoteActivity', () => extFiles.resumeAll()));
    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.suspendRemoteActivity', () => extFiles.suspendAll()));
    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.clearRemoteActivityCache', () => extFiles.clearCache()));
    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.clearRemoteActivityCacheForService', () =>
        pickUser('Select service', extFiles.currentlyAvailableServices().map(x => { return { label: x, value: x }; })).then(x => extFiles.clearCache(x), () => { })
    ));
}

async function registerToContextWithClient(context: vscode.ExtensionContext, client: vscodelc.BaseLanguageClient, telemetry: Telemetry) {
    context.subscriptions.push(vscode.languages.registerCodeActionsProvider([languageIdHlasm, hlasmplugin_folder_filter, bridge_json_filter], new HLASMCodeActionsProvider(client)));

    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.toggleAdvisoryConfigurationDiagnostics', () => toggleAdvisoryConfigurationDiagnostics(client)));

    context.subscriptions.push(vscode.workspace.registerTextDocumentContentProvider(schemeVirtualFiles, new HLASMVirtualFileContentProvider(client)));

    context.subscriptions.push(vscode.commands.registerCommand("extension.hlasm-plugin.downloadDependencies", (...args: any[]) => downloadDependencies(context, telemetry, client.outputChannel, ...args)));

    context.subscriptions.push(vscode.commands.registerTextEditorCommand('extension.hlasm-plugin.showOutput', showOutputCommand));

    activateBranchDecorator(context, client);
}
