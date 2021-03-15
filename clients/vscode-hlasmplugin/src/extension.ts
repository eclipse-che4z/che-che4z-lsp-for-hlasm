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


import { SemanticTokensFeature } from './semanticTokens';
import { HLASMConfigurationProvider, getCurrentProgramName, getProgramName } from './debugProvider';
import { ContinuationHandler } from './continuationHandler';
import { CustomEditorCommands } from './customEditorCommands';
import { EventsHandler, getConfig } from './eventsHandler';
import { ServerFactory, ServerCommunicationMethod } from './serverFactory';
import { HLASMDebugAdapterFactory } from './hlasmDebugAdapterFactory';

const offset = 71;
const continueColumn = 15;
//export var hlasmpluginClient : vscodelc.LanguageClient;
/**
 * ACTIVATION
 * activates the extension
 */
export async function activate(context: vscode.ExtensionContext) {
    // patterns for files and configs
    const filePattern: string = '**/*';

    // create client options
    const syncFileEvents = getConfig<boolean>('syncFileEvents', true);
    const clientOptions: vscodelc.LanguageClientOptions = {
        documentSelector: [{ language: 'hlasm' }],
        synchronize: !syncFileEvents ? undefined : {
            fileEvents: vscode.workspace.createFileSystemWatcher(filePattern)
        },
        uriConverters: {
            code2Protocol: (value: vscode.Uri) => value.toString(),
            protocol2Code: (value: string) =>
                vscode.Uri.file((vscode.Uri.parse(value).fsPath))
        }
    };

    // create server options
    var factory = new ServerFactory();
    const commMethod = getConfig<ServerCommunicationMethod>('communicationMethod', 'native');
    const serverOptions = await factory.create(commMethod);

    //client init
    var hlasmpluginClient = new vscodelc.LanguageClient('Hlasmplugin Language Server', serverOptions, clientOptions);

    //asm contribution 
    var highlight = new SemanticTokensFeature(hlasmpluginClient);
    // register highlighting as features
    hlasmpluginClient.registerFeature(highlight);
    // register all commands and objects to context
    await registerToContext(context, highlight, hlasmpluginClient);
    //give the server some time to start listening when using TCP
    setTimeout(function () {
        hlasmpluginClient.start();
    }, (commMethod === 'tcp') ? 2000 : 0);

    let api = {
        getExtension(): vscodelc.LanguageClient {
            return hlasmpluginClient;
        }
    };
    return api;
}

async function registerToContext(context: vscode.ExtensionContext, highlight: SemanticTokensFeature, client: vscodelc.LanguageClient) {
    const completeCommand = "editor.action.triggerSuggest";
    var commandList = await vscode.commands.getCommands();

    // check whether the continuation commands have already been registered
    var commandsRegistered = commandList.find(command => command == 'insertContinuation' || command == 'removeContinuation');

    // initialize helpers
    const handler = new EventsHandler(completeCommand, highlight);
    const contHandling = new ContinuationHandler();
    const commands = new CustomEditorCommands();

    // register them
    context.subscriptions.push(contHandling);
    context.subscriptions.push(commands);
    context.subscriptions.push(handler);

    // register provider for all hlasm debug configurations
    context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('hlasm', new HLASMConfigurationProvider()));
    context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('hlasm', new HLASMDebugAdapterFactory(client)));

    // register continuation handlers
    if (!commandsRegistered) {
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("insertContinuation",
            (editor, edit) => contHandling.insertContinuation(editor, edit, offset, continueColumn)));
        context.subscriptions.push(vscode.commands.registerTextEditorCommand("removeContinuation",
            (editor, edit) => contHandling.removeContinuation(editor, edit, offset)));
    }

    // overrides should happen only if the user wishes
    if (getConfig<boolean>('continuationHandling', false)) {
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
    }
    // register event handlers
    context.subscriptions.push(vscode.workspace.onDidChangeTextDocument(e => handler.onDidChangeTextDocument(e, highlight, offset)));
    context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(e => handler.onDidOpenTextDocument(e, highlight)));
    context.subscriptions.push(vscode.workspace.onDidChangeConfiguration(e => handler.onDidChangeConfiguration(e)));
    context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(e => handler.onDidSaveTextDocument(e)));
    context.subscriptions.push(vscode.window.onDidChangeVisibleTextEditors(e => handler.onDidChangeVisibleTextEditors(e, highlight)));
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(e => handler.onDidChangeActiveTextEditor(e)));

    // register filename retrieve functions for debug sessions
    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.getProgramName', () => getProgramName()));
    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.getCurrentProgramName', () => getCurrentProgramName()));

    return handler;
}