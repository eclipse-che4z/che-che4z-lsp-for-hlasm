import * as vscode from 'vscode';
import * as vscodelc from 'vscode-languageclient';
import * as path from 'path'
import { SemanticHighlightingFeature } from './semanticHighlighting'
/**
 * Method to get workspace configuration option
 * @param option name of the option (e.g. for hlasmplugin.path should be path)
 * @param defaultValue default value to return if option is not set
 */
function getConfig<T>(option: string, defaultValue?: any): T {
    const config = vscode.workspace.getConfiguration('hlasmplugin');
    return config.get<T>(option, defaultValue);
}

var highlight: SemanticHighlightingFeature;

function findSpace(textLine: vscode.TextLine, currentPosition: number, continuationPosition: number)
{
    var text = textLine.text.substring(currentPosition, continuationPosition);
    var spacePosition = text.lastIndexOf(" ");
    return (spacePosition == -1) ? null : new vscode.Range(new vscode.Position(textLine.lineNumber, spacePosition+currentPosition), new vscode.Position(textLine.lineNumber, spacePosition+currentPosition+1));
}
/**
 *  this method is called when your extension is activate
 *  your extension is activated the very first time the command is executed
 */
export function activate(context: vscode.ExtensionContext) {
    const syncFileEvents = getConfig<boolean>('syncFileEvents', true);

    const server: vscodelc.Executable = {
        command: path.join(__dirname, '..', 'bin', 'language_server'),
        args: getConfig<string[]>('arguments')
    };
    const serverOptions: vscodelc.ServerOptions = server;

    const filePattern: string = '**/*'
    const configPattern: string = '**/{' + ['proc_grps.json', 'pgm_conf.json'].join() + '}';
    
    const clientOptions: vscodelc.LanguageClientOptions = {
        // 
        documentSelector: [{ language:'hlasm'}, {pattern:configPattern}],
        synchronize: !syncFileEvents ? undefined : {
            fileEvents: vscode.workspace.createFileSystemWatcher(filePattern)
        },
        uriConverters: {
            code2Protocol: (value: vscode.Uri) => value.toString(),
            protocol2Code: (value: string) =>
                vscode.Uri.file((vscode.Uri.parse(value).fsPath))
        }
    };

    var hlasmpluginClient = new vscodelc.LanguageClient('Hlasmplugin Language Server', serverOptions, clientOptions);
    highlight = new SemanticHighlightingFeature(hlasmpluginClient);
    hlasmpluginClient.registerFeature(highlight);
    console.log('Hlasmplugin Language Server is now active!');

    const disposable = hlasmpluginClient.start();

    var editor: vscode.TextEditor;
    var currentPosition: vscode.Position;
    var continuationOffset: number;

    function initialize(globalContinuationOffset: boolean = false)
    {
        editor = vscode.window.activeTextEditor;
        currentPosition = editor.selection.active;
        continuationOffset = highlight.getContinuation((globalContinuationOffset) ? -1 : currentPosition.line, editor.document.uri.toString());
    }

    function setCursor(position: vscode.Position)
    {
        editor.selection = new vscode.Selection(position, position);
    }

    vscode.commands.registerCommand("type", (args: {text: string}) => {
        initialize();
        if (currentPosition.line != editor.selection.anchor.line || currentPosition.character != editor.selection.anchor.character)
        {
            editor.edit(function (edit: vscode.TextEditorEdit) {
                if (continuationOffset != -1 && currentPosition.character < continuationOffset) {
                    var spaceRange = findSpace(editor.document.lineAt(currentPosition), currentPosition.character, continuationOffset);
                    if (spaceRange)
                        edit.delete(spaceRange);
                } 
                if (editor.selection.anchor.line > editor.selection.active.line)
                {
                    edit.insert(editor.selection.active, args.text);
                }
                else if (editor.selection.anchor.line < editor.selection.active.line)
                {
                    edit.insert(editor.selection.anchor, args.text);
                }
                else
                {
                    if (editor.selection.anchor.character < editor.selection.active.character)
                    {
                        edit.insert(editor.selection.anchor, args.text);
                    }
                    else
                    {
                        edit.insert(editor.selection.active, args.text);
                    }
                }
                var continuationPosition = highlight.getContinuation(currentPosition.line, editor.document.uri.toString());
                var selectionSize = (currentPosition.character - editor.selection.anchor.character != 0) ? currentPosition.character - editor.selection.anchor.character : 1;
                var newCursorPosition = new vscode.Position(currentPosition.line, currentPosition.character - ((selectionSize > 0 && currentPosition.character > 0) ? selectionSize : 0));
                if (continuationPosition != -1 && currentPosition.character <= continuationPosition && currentPosition.character > 0 && editor.selection.isSingleLine) {
                    var beforeContinuationChars = new vscode.Range(currentPosition.line, continuationPosition - Math.abs(selectionSize), currentPosition.line, continuationPosition);
                    edit.insert(beforeContinuationChars.end, " ".repeat(Math.abs(selectionSize)));
                }
                else if (currentPosition.character == 0 && currentPosition.line > 0) {
                    newCursorPosition = new vscode.Position(currentPosition.line - 1, editor.document.lineAt(currentPosition.line - 1).text.length);
                }
                edit.delete(new vscode.Range(currentPosition, (editor.selection.anchor.character == currentPosition.character && editor.selection.anchor.line == currentPosition.line) ?
                    newCursorPosition :
                    new vscode.Position(editor.selection.anchor.line, editor.selection.anchor.character)));
            });
        }
        else
        editor.edit(function (edit: vscode.TextEditorEdit) {
            if (continuationOffset != -1 && currentPosition.character < continuationOffset) {
                var spaceRange = findSpace(editor.document.lineAt(currentPosition), currentPosition.character, continuationOffset);
                if (spaceRange)
                    edit.delete(spaceRange);
            }    
            edit.insert(currentPosition, args.text);
        });
    });
    vscode.commands.registerCommand("deleteLeftCont", () => {
        initialize();
        var selectionSize = (currentPosition.character - editor.selection.anchor.character != 0) ? currentPosition.character - editor.selection.anchor.character : 1;
        var newCursorPosition = new vscode.Position(currentPosition.line, currentPosition.character - ((selectionSize > 0 && currentPosition.character > 0) ? selectionSize : 0));
        editor.edit(function (edit: vscode.TextEditorEdit) {
            if (continuationOffset != -1 && currentPosition.character <= continuationOffset && currentPosition.character > 0 && editor.selection.isSingleLine) {
                var beforeContinuationChars = new vscode.Range(currentPosition.line, continuationOffset - Math.abs(selectionSize), currentPosition.line, continuationOffset);
                edit.insert(beforeContinuationChars.end, " ".repeat(Math.abs(selectionSize)));
            }
            else if (currentPosition.character == 0 && currentPosition.line > 0) {
                newCursorPosition = new vscode.Position(currentPosition.line - 1, editor.document.lineAt(currentPosition.line - 1).text.length);
            }
            edit.delete(new vscode.Range(currentPosition, (editor.selection.anchor.character == currentPosition.character && editor.selection.anchor.line == currentPosition.line) ?
                newCursorPosition :
                new vscode.Position(editor.selection.anchor.line, editor.selection.anchor.character)));
        });
        if (currentPosition.character == continuationOffset)
            setCursor(newCursorPosition);
    });
    vscode.commands.registerCommand("deleteRightCont", () => {
        initialize();
        var selectionSize = (currentPosition.character - editor.selection.anchor.character != 0) ? currentPosition.character - editor.selection.anchor.character : -1;
        var newCursorPosition = new vscode.Position(currentPosition.line, currentPosition.character - ((selectionSize >= -1) ? selectionSize : 0));
        editor.edit(function (edit: vscode.TextEditorEdit) {
            if (continuationOffset != -1 && currentPosition.character < continuationOffset && editor.selection.isSingleLine) {
                var beforeContinuationChars = new vscode.Range(currentPosition.line, continuationOffset - Math.abs(selectionSize), currentPosition.line, continuationOffset);
                edit.insert(beforeContinuationChars.end, " ".repeat(Math.abs(selectionSize)));
            }
            if (editor.document.lineAt(currentPosition).text.substring(currentPosition.character) == "")
                newCursorPosition = new vscode.Position(currentPosition.line + 1, 0);
            edit.delete(new vscode.Range(currentPosition, (editor.selection.anchor.character == currentPosition.character && editor.selection.anchor.line == currentPosition.line) ?
                newCursorPosition :
                new vscode.Position(editor.selection.anchor.line, editor.selection.anchor.character)));
        });
    });
    vscode.commands.registerCommand("insertContinuation", () => {
        initialize(true);
        var continueColumn = highlight.getContinueColumn(editor.document.uri.toString());
        if (highlight.getContinuation(currentPosition.line, editor.document.uri.toString()) == -1) {
            editor.edit(function (edit) {
                if (currentPosition.character < continuationOffset) {
                    var lastChar = editor.document.lineAt(currentPosition).text.length;
                    lastChar = (lastChar < continuationOffset) ? lastChar : continuationOffset;
                    edit.insert(new vscode.Position(currentPosition.line, lastChar), " ".repeat(continuationOffset - lastChar));
                }
                var continuationPosition = new vscode.Position(currentPosition.line, continuationOffset);
                edit.replace(new vscode.Range(continuationPosition, new vscode.Position(currentPosition.line, continuationOffset + 1)), "X");
                edit.insert(new vscode.Position(currentPosition.line, editor.document.lineAt(currentPosition).text.length), "\r\n".concat(" ".repeat(continueColumn)));
            });
            setCursor(new vscode.Position(currentPosition.line + 1, continueColumn));
        }
        else
        {
            editor.edit(function (edit) {
                edit.insert(new vscode.Position(currentPosition.line, editor.document.lineAt(currentPosition).text.length), "\r\n".concat(" ".repeat(continuationOffset).concat("X")));
            });
        }
    });
    vscode.commands.registerCommand("removeContinuation", () => {
        initialize(true);
        if (currentPosition.line > 0 && highlight.getContinuation(currentPosition.line - 1, editor.document.uri.toString()) != -1) {
            var continuationPosition = new vscode.Position(currentPosition.line - 1, continuationOffset);
            editor.edit(function (edit) {
                edit.delete(new vscode.Range(new vscode.Position(currentPosition.line, editor.document.lineAt(currentPosition).text.length), new vscode.Position(currentPosition.line - 1, editor.document.lineAt(currentPosition.line - 1).text.length)));
                edit.replace(new vscode.Range(continuationPosition, new vscode.Position(continuationPosition.line, continuationOffset + 1)), " ");
            });
            editor.selection = new vscode.Selection(continuationPosition, continuationPosition);
        }
    });
}

vscode.window.onDidChangeVisibleTextEditors(() => {
    highlight.applyDecorations();
});

vscode.workspace.onDidChangeConfiguration( (change: vscode.ConfigurationChangeEvent) => {
    if (change.affectsConfiguration("hlasmplugin.highlightColors"))
      {highlight.updateColors();}
})
