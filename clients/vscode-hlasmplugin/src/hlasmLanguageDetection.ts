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

import { ConfigurationsHandler } from './configurationsHandler'

/**
 * Runs automatic HLASM language detection on given files
 */
export class HLASMLanguageDetection {
    private configSetup: ConfigurationsHandler;
    // regex to match the most common HLASM instructions as a first/second word in line
    private readonly referenceInstructions: RegExp;
    // regex to match MACRO instruction
    private readonly macroInstruction: RegExp;
    /**
     * @param configSetup Used for the stored regex expressions 
     */
    constructor(configSetup: ConfigurationsHandler) {
        this.configSetup = configSetup;
        this.referenceInstructions = new RegExp("^\\S*( |\\t)+(ICTL|\\*PROCESS|END|COND|IC|ICM|L|LA|LCR|LH|LHI|LM|LNR|LPR|LR|LTR|MVC|MVCL|MVI|ST|STC|STCM|STH|STM|A|AH|AHI|AL|ALR|AR|C|CH|CR|D|DR|M|MH|MHI|MR|S|SH|SL|SLR|SR|CL|CLC|CLCL|CLI|CLM|CLR|N|NC|NI|NR|O|OC|OI|OR|SLA|SLDA|SLDL|SLDA|SLL|SRA|SRDA|SRDL|SRL|TM|X|XC|XI|XR|BAL|BALR|BAS|BASR|BC|BCR|BCT|BCTR|BXH|BXLE|AP|CP|CVB|CVD|DP|ED|EDMK|MP|MVN|MVO|MVZ|PACK|SP|SRP|UNPK|ZAP|CDS|CS|EX|STCK|SVC|TR|TRT|TS|B|J|NOP|BE|BNE|BL|BNL|BH|BHL|BZ|BNZ|BM|BNM|AMODE|CSECT|DC|DS|DSECT|DROP|EJECT|END|EQU|LTORG|ORG|POP|PRINT|PUSH|RMODE|SPACE|USING|TITLE|BP|BNP|BO|BNO|ABEND|CALL|CLOSE|DCB|GET|OPEN|PUT|RETURN|SAVE|STORAGE|COPY|GBLC|GBLB|SETA|SETB|SETC)( |\\t)+");
        this.macroInstruction = new RegExp("^( |\\t)+MACRO( |\\t)*");
    }

    //automatic detection function
    setHlasmLanguage(document: vscode.TextDocument): boolean {
        // check only plain text files
        if (document.languageId == 'plaintext') {
            if (this.checkHlasmLanguage(document)) {
                vscode.languages.setTextDocumentLanguage(document, 'hlasm');
                return true;
            }
        }
        return document.languageId == 'hlasm';
    }

    private checkHlasmLanguage(document: vscode.TextDocument) {
        // check if the current active editor document matches any of the wildcards
        if (this.configSetup.match(document.uri.fsPath))
            return true;

        const text = document.getText();
        if (text.length == 0)
            return false;

        var score = 0;
        var lines = 0;
        var lastContinued = false;
        //iterate line by line
        const split = text.split('\n');
        //check whether the first line is MACRO and immediately set to hlasm
        if (this.macroInstruction.test(split[0].toUpperCase()))
            return true;

        split.forEach(line => {
            // irrelevant line, remove from total count comments starting "*" and ".*"
            if (line != "" && !line.startsWith("*") && !line.startsWith(".*")) {
                lines++;
                //test if line contains reference instruction
                if ((this.referenceInstructions.test(line.toUpperCase()) || lastContinued) && line.length <= 80) {
                    score++;
                    // naive continuation check
                    if (line.length > 71 && line[71] != " ")
                        lastContinued = true;
                    else
                        lastContinued = false;
                }
            }
        });
        //final score is the ratio between instruction line and all document lines
        return score / lines > 0.4;
    }
}
