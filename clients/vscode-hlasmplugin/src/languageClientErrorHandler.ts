import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient';

import { ErrorHandler } from 'vscode-languageclient';
import { Telemetry } from './telemetry';

const extensionID : string = "broadcommfd.hlasm-language-support";

export class LanguageClientErrorHandler implements ErrorHandler
{
    defaultHandler : ErrorHandler = undefined;
    telemetry : Telemetry;
    
    constructor(tlmtry : Telemetry)
    {
        this.telemetry = tlmtry;
    }
    
    error(error: Error, message: vscodelc.Message, count: number): vscodelc.ErrorAction {
        this.telemetry.reportEvent("hlasm.connectionError", error)
        
        return this.defaultHandler.error(error, message, count);
    }
    closed(): vscodelc.CloseAction {
        this.telemetry.reportEvent("hlasm.connectionClosed")
        return this.defaultHandler.closed();
    }
    
}