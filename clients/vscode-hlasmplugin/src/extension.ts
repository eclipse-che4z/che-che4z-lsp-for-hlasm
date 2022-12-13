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
import * as vscodelc from 'vscode-languageclient/node';


import { HLASMConfigurationProvider, getCurrentProgramName, getProgramName } from './debugProvider';
import { ContinuationHandler } from './continuationHandler';
import { CustomEditorCommands } from './customEditorCommands';
import { EventsHandler, getConfig } from './eventsHandler';
import { ServerFactory, ServerVariant } from './serverFactory';
import { HLASMDebugAdapterFactory } from './hlasmDebugAdapterFactory';
import { Telemetry } from './telemetry';
import { LanguageClientErrorHandler } from './languageClientErrorHandler';
import { HLASMVirtualFileContentProvider } from './hlasmVirtualFileContentProvider';
import { downloadDependencies } from './hlasmDownloadCommands';
import { blockCommentCommand, CommentOption, lineCommentCommand } from './commentEditorCommands';
import { HLASMCodeActionsProvider } from './hlasmCodeActionsProvider';
import { hlasmplugin_folder } from './constants';
import { ConfigurationsHandler } from './configurationsHandler';

export const EXTENSION_ID = "broadcommfd.hlasm-language-support";

const offset = 71;
const continueColumn = 15;

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

/**
 * ACTIVATION
 * activates the extension
 */
export async function activate(context: vscode.ExtensionContext) {
    const serverVariant = getConfig<ServerVariant>('serverVariant', 'native');

    const telemetry = new Telemetry();
    context.subscriptions.push(telemetry);

    // setTimeout is needed, because telemetry initialization is asynchronous
    // and AFAIK no event in the API is exposed to send the activation telemetry event
    setTimeout(() => { telemetry.reportEvent("hlasm.activated", { server_variant: serverVariant.toString() }); }, 1000);

    // patterns for files and configs
    const filePattern: string = '**/*';

    const clientErrorHandler = new LanguageClientErrorHandler(telemetry);

    // create client options
    const syncFileEvents = getConfig<boolean>('syncFileEvents', true);
    const clientOptions: vscodelc.LanguageClientOptions = {
        documentSelector: [{ language: 'hlasm' }],
        synchronize: !syncFileEvents ? undefined : {
            fileEvents: [
                vscode.workspace.createFileSystemWatcher(filePattern),
                vscode.workspace.createFileSystemWatcher(hlasmplugin_folder + '/*.json'),
            ]
        },
        errorHandler: clientErrorHandler
    };


    // create server options
    var factory = new ServerFactory();

    const serverOptions = await factory.create(serverVariant);

    //client init
    var hlasmpluginClient = new vscodelc.LanguageClient('Hlasmplugin Language Server', serverOptions, clientOptions);

    clientErrorHandler.defaultHandler = hlasmpluginClient.createDefaultErrorHandler();
    // The objectToString is necessary, because telemetry reporter only takes objects with
    // string properties and there are some boolean that we receive from the language server
    hlasmpluginClient.onTelemetry((object) => { telemetry.reportEvent(object.method_name, objectToString(object.properties), object.measurements) });

    // register all commands and objects to context
    await registerToContext(context, hlasmpluginClient, telemetry);

    //give the server some time to start listening when using TCP
    if (serverVariant === 'tcp')
        await sleep(2000);

    context.subscriptions.push(hlasmpluginClient.start());

    if (serverVariant === 'native')
        startCheckingNativeClient(hlasmpluginClient);

    let api = {
        getExtension(): vscodelc.LanguageClient {
            return hlasmpluginClient;
        },
        getTelemetry(): Telemetry {
            return telemetry;
        }
    };
    return api;
}

function startCheckingNativeClient(hlasmpluginClient: vscodelc.LanguageClient) {
    const timeout = setTimeout(() => {
        const use_wasm = 'Switch to WASM version';
        vscode.window.showWarningMessage('The language server did not start.', ...[use_wasm, 'Ignore']).then((value) => {
            if (value === use_wasm) {
                vscode.workspace.getConfiguration('hlasm').update('serverVariant', 'wasm', vscode.ConfigurationTarget.Global).then(
                    () => {
                        const reload = 'Reload window';
                        vscode.window.showInformationMessage('User settings updated.', ...[reload]).then((value) => {
                            if (value === reload)
                                vscode.commands.executeCommand('workbench.action.reloadWindow');
                        })
                    },
                    (error) => {
                        vscode.window.showErrorMessage(error);
                    });
            }
        })
    }, 15000);
    hlasmpluginClient.onReady().then(() => {
        clearTimeout(timeout);
    });
}

async function registerToContext(context: vscode.ExtensionContext, client: vscodelc.LanguageClient, telemetry: Telemetry) {
    const completeCommand = "editor.action.triggerSuggest";
    const commandList = await vscode.commands.getCommands();

    // check whether the continuation commands have already been registered
    const commandsRegistered = commandList.find(command => command == "extension.hlasm-plugin.insertContinuation" || command == "extension.hlasm-plugin.removeContinuation");

    // initialize helpers
    const handler = new EventsHandler(completeCommand);
    const contHandling = new ContinuationHandler();
    const commands = new CustomEditorCommands();

    // register them
    context.subscriptions.push(contHandling);
    context.subscriptions.push(commands);
    context.subscriptions.push(handler);

    // register provider for all hlasm debug configurations
    context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('hlasm', new HLASMConfigurationProvider()));
    context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('hlasm', new HLASMDebugAdapterFactory(client)));
    context.subscriptions.push(vscode.languages.registerCodeActionsProvider('hlasm', new HLASMCodeActionsProvider(client)));

    // register continuation handlers
    if (!commandsRegistered) {
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.insertContinuation",
            (editor, edit) => contHandling.insertContinuation(editor, edit, offset, continueColumn)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.removeContinuation",
            (editor, edit) => contHandling.removeContinuation(editor, edit, offset)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.rearrangeSequenceNumbers",
            (editor, edit) => contHandling.rearrangeSequenceNumbers(editor, edit, offset)));

        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.toggleCommentEditorCommands",
            (editor, edit) => lineCommentCommand(editor, edit, CommentOption.toggle)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.addCommentEditorCommands",
            (editor, edit) => lineCommentCommand(editor, edit, CommentOption.add)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.removeCommentEditorCommands",
            (editor, edit) => lineCommentCommand(editor, edit, CommentOption.remove)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("extension.hlasm-plugin.blockCommentEditorCommands",
            (editor, edit) => blockCommentCommand(editor, edit)));
    }

    // overrides should happen only if the user wishes
    if (getConfig<boolean>('continuationHandling', false)) {
        try {
            context.subscriptions.push(vscode.commands.registerTextEditorCommand("type",
                (editor, edit, args) => commands.insertChars(editor, edit, args, offset)));
            context.subscriptions.push(vscode.commands.registerTextEditorCommand("paste",
                (editor, edit, args) => commands.insertChars(editor, edit, args, offset)));
            context.subscriptions.push(vscode.commands.registerTextEditorCommand("cut",
                (editor, edit) => commands.cut(editor, edit, offset)));
            context.subscriptions.push(vscode.commands.registerTextEditorCommand("deleteLeft",
                (editor, edit) => commands.deleteLeft(editor, edit, offset)));
            context.subscriptions.push(vscode.commands.registerTextEditorCommand("deleteRight",
                (editor, edit) => commands.deleteRight(editor, edit, offset)));
        } catch (e) { /* just ignore */ }
    }
    // register event handlers
    context.subscriptions.push(vscode.workspace.onDidChangeTextDocument(e => handler.onDidChangeTextDocument(e, offset)));
    context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(e => handler.onDidOpenTextDocument(e)));
    context.subscriptions.push(vscode.workspace.onDidChangeConfiguration(e => handler.onDidChangeConfiguration(e)));
    context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(e => handler.onDidSaveTextDocument(e)));
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(e => handler.onDidChangeActiveTextEditor(e)));
    context.subscriptions.push(vscode.workspace.onDidChangeWorkspaceFolders(e => handler.onDidChangeWorkspaceFolders(e)));

    // register filename retrieve functions for debug sessions
    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.getProgramName', () => getProgramName()));
    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.getCurrentProgramName', () => getCurrentProgramName()));

    context.subscriptions.push(vscode.workspace.registerTextDocumentContentProvider("hlasm", new HLASMVirtualFileContentProvider(client)));

    context.subscriptions.push(vscode.commands.registerCommand("extension.hlasm-plugin.downloadDependencies", (...args: any[]) => downloadDependencies(context, telemetry, ...args)));

    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.createCompleteConfig', ConfigurationsHandler.createCompleteConfig));

    return handler;
}
