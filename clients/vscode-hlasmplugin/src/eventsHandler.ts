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

import { ContinuationDocumentsInfo } from './hlasmSemanticHighlighting'
import { ConfigurationsHandler } from './configurationsHandler'
import { HLASMLanguageDetection } from './hlasmLanguageDetection'
import { SemanticHighlightingFeature } from './semanticHighlighting';

/**
 * Handles various events happening in VSCode
 */
export class EventsHandler {
    private readonly isInstruction: RegExp;
    private readonly isTrigger: RegExp;
    private readonly completeCommand: string;
    // several events update/need configuration information
    private configSetup: ConfigurationsHandler;
    // newly open files are detected for HLASM
    private langDetect: HLASMLanguageDetection;
    // parse in progress indicator
    /**
     * 
     * @param completeCommand Used to invoke complete manually in continuationHandling mode
     * @param highlight Shows/hides parsing progress
     */
    constructor(completeCommand: string)
    {
        this.isInstruction = new RegExp("^([^*][^*]\\S*\\s+\\S+|\\s+\\S*)$");
        this.isTrigger = new RegExp("^[a-zA-Z\*]+$");
        this.completeCommand = completeCommand;
        this.configSetup = new ConfigurationsHandler();
        this.langDetect = new HLASMLanguageDetection(this.configSetup);
        this.initialize();
    }

    dispose() {}

    // invoked on extension activation
    private initialize()
    {
        // initialize wildcards
        this.configSetup.updateWildcards();
        // first run, check for assembler language and configurations
        if (vscode.window.activeTextEditor) {
            this.editorChanged(vscode.window.activeTextEditor.document);
        }
    }

    // when contents of a document change, issue a completion request
    onDidChangeTextDocument(event: vscode.TextDocumentChangeEvent, info: ContinuationDocumentsInfo): boolean {
        if (getConfig<boolean>('continuationHandling', false)) {
            if (event.document.languageId != 'hlasm')
                return false;

            //const editor = vscode.window.activeTextEditor;
            if (event.contentChanges.length == 0 || event.document.languageId != "hlasm")
                return false;

            const change = event.contentChanges[0];
            const currentLine = event.document.getText(
                new vscode.Range(
                    new vscode.Position(
                        change.range.start.line, 0),
                        change.range.start));
                        
            const foundDoc = info.get(event.document.uri.toString());
            const notContinued =
                change.range.start.line == 0 ||
                !foundDoc ||
                !foundDoc.lineContinuations.get(change.range.start.line - 1);

            if ((currentLine != "" &&
                this.isTrigger.test(change.text) &&
                this.isInstruction.test(currentLine) &&
                notContinued &&
                currentLine[0] != "*") || change.text == "." || change.text == "&") {
                vscode.commands.executeCommand(this.completeCommand);
                return true;
            }
        }
        return false;
    }

    // when any visible text editor changes, apply decorations for it
    onDidChangeVisibleTextEditors(editors: vscode.TextEditor[], highlight: SemanticHighlightingFeature) {
        for (var i = 0; i < editors.length; i++) {
            if (editors[i].document.languageId == 'hlasm') {
                highlight.colorize();
                break;
            }
        }
    }

    // when document opens, show parse progress
    onDidOpenTextDocument(document: vscode.TextDocument) {
        //this.showProgress(document);
        this.editorChanged(document);
    }

    onDidChangeConfiguration(event: vscode.ConfigurationChangeEvent) {
        if (event.affectsConfiguration("hlasmplugin.continuationHandling"))
            vscode.commands.executeCommand("workbench.action.reloadWindow");
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