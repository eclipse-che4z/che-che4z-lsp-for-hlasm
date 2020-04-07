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

import { HLASMSemanticHighlightingFeature } from './hlasmSemanticHighlighting'
import { LinePositionsInfo } from './customEditorCommands'
import { ConfigurationsHandler } from './configurationsHandler'
import { HLASMLanguageDetection } from './hlasmLanguageDetection'

/**
 * Handles various events happening in VSCode
 */
export class EventsHandler {
    private readonly isInstruction: RegExp;
    private readonly isTrigger: RegExp;
    private readonly completeCommand: string;
    private highlight: HLASMSemanticHighlightingFeature;
    // several events update/need configuration information
    private configSetup: ConfigurationsHandler;
    // newly open files are detected for HLASM
    private langDetect: HLASMLanguageDetection;
    /**
     * 
     * @param completeCommand Used to invoke complete manually in continuationHandling mode
     * @param highlight Shows/hides parsing progress
     */
    constructor(completeCommand: string, highlight: HLASMSemanticHighlightingFeature)
    {
        this.isInstruction = new RegExp("^([^*][^*]\\S*\\s+\\S+|\\s+\\S*)$");
        this.isTrigger = new RegExp("^[a-zA-Z\*]+$");
        this.completeCommand = completeCommand;
        this.highlight = highlight;
        this.configSetup = new ConfigurationsHandler();
        this.langDetect = new HLASMLanguageDetection(this.configSetup);
        this.initialize();
    }

    dispose() {
        this.highlight.dispose();
    }

    // invoked on extension activation
    private initialize()
    {
        // initialize wildcards
        this.configSetup.updateWildcards();
        // first run, check for assembler language and configurations
        if (vscode.window.activeTextEditor) {
            this.highlight.showProgress(vscode.window.activeTextEditor.document);
            this.editorChanged(vscode.window.activeTextEditor.document);
        }
    }

    // when contents of a document change, issue a completion request
    onDidChangeTextDocument(event: vscode.TextDocumentChangeEvent) {
        if (getConfig<boolean>('continuationHandling', false)) {
            if (event.document.languageId != 'hlasm')
                return;

            const editor = vscode.window.activeTextEditor;
            if (event.contentChanges.length == 0 || editor.document.languageId != "hlasm")
                return;

            const change = event.contentChanges[0].text;
            const info = new LinePositionsInfo(
                editor.selection.active,
                this.highlight.getContinuation(editor.selection.active.line, editor.document.uri.toString())
            );

            const currentLine = editor.document.getText(
                new vscode.Range(
                    new vscode.Position(info.currentPosition.line, 0),
                    info.currentPosition));

            const notContinued =
                info.currentPosition.line == 0 ||
                this.highlight.getContinuation(info.currentPosition.line - 1, editor.document.uri.toString()) == -1;

            if ((currentLine != "" &&
                this.isTrigger.test(change) &&
                this.isInstruction.test(currentLine) &&
                notContinued &&
                currentLine[0] != "*") || change == "." || change == "&") {
                vscode.commands.executeCommand(this.completeCommand);
            }
        }
        this.highlight.showProgress(event.document);
    }

    // when files closes before it was succesfuly parsed, remove it from the parsing list
    onDidCloseTextDocument(document: vscode.TextDocument) {
        this.highlight.hideProgress(document.uri.toString());
    }

    // when document opens, show parse progress
    onDidOpenTextDocument(document: vscode.TextDocument) {
        this.highlight.showProgress(document);
        this.editorChanged(document);
    }

    onDidChangeConfiguration(event: vscode.ConfigurationChangeEvent) {
        if (event.affectsConfiguration("hlasmplugin.continuationHandling"))
            vscode.commands.executeCommand("workbench.action.reloadWindow");
    }

    // when any visible text editor changes, apply decorations for it
    onDidChangeVisibleTextEditors(editors: vscode.TextEditor[]) {
        for (var i = 0; i < editors.length; i++) {
            if (editors[i].document.languageId == 'hlasm') {
                this.highlight.colorize();
                break;
            }
        };
    }

    // when active editor changes, try to set a language for it
    onDidChangeActiveTextEditor(editor: vscode.TextEditor) {
        if (editor)
            this.editorChanged(editor.document);
    }

    // when pgm_conf changes, update wildcards
    onDidSaveTextDocument(document: vscode.TextDocument) {
        this.configSetup.updateWildcards(document.fileName);
    }

    // should the configs be checked
    private editorChanged(document: vscode.TextDocument) {
        if (this.configSetup.shouldCheckConfigs && this.langDetect.setHlasmLanguage(document))
            this.configSetup.checkConfigs();
    }
}

/**
 * Method to get workspace configuration option
 * @param option name of the option (e.g. for hlasmplugin.path should be path)
 * @param defaultValue default value to return if option is not set
 */
export function getConfig<T>(option: string, defaultValue?: any): T {
    const config = vscode.workspace.getConfiguration('hlasmplugin');
    return config.get<T>(option, defaultValue);
}