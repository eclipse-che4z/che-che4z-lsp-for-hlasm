import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient';
import { realpathSync } from 'fs';

/**
 * Method to get workspace configuration option
 * @param option name of the option (e.g. for hlasmplugin.path should be path)
 * @param defaultValue default value to return if option is not set
 */
function getConfig<T>(option: string, defaultValue?: any): T {
    const config = vscode.workspace.getConfiguration('hlasmplugin');
    return config.get<T>(option, defaultValue);
}

/**
 *  this method is called when your extension is activate
 *  your extension is activated the very first time the command is executed
 */
export function activate(context: vscode.ExtensionContext) {
    const syncFileEvents = getConfig<boolean>('syncFileEvents', true);

    const server: vscodelc.Executable = {
        command: getConfig<string>('path'),
        args: getConfig<string[]>('arguments')
    };
    const serverOptions: vscodelc.ServerOptions = server;

    const filePattern: string = '**/*.{' +
        ['txt', 'hlasm'].join() + '}';
    const clientOptions: vscodelc.LanguageClientOptions = {
        // 
        documentSelector: [{ scheme: 'file', pattern: filePattern }],
        synchronize: !syncFileEvents ? undefined : {
            fileEvents: vscode.workspace.createFileSystemWatcher(filePattern)
        },
        uriConverters: {
            code2Protocol: (value: vscode.Uri) => value.toString(),
            protocol2Code: (value: string) =>
                vscode.Uri.file(realpathSync(vscode.Uri.parse(value).fsPath))
        }
    };

    const hlasmpluginClient = new vscodelc.LanguageClient('Hlasmplugin Language Server', serverOptions, clientOptions);
    console.log('Hlasmplugin Language Server is now active!');

    const disposable = hlasmpluginClient.start();
}
