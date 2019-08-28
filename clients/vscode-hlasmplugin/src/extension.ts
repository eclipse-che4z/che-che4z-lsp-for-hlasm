import * as vscode from 'vscode';
import { WorkspaceFolder, DebugConfiguration, ProviderResult, CancellationToken } from 'vscode';
import * as vscodelc from 'vscode-languageclient';
import * as path from 'path'
import * as Net from 'net';
import { ASMSemanticHighlightingFeature } from './ASMsemanticHighlighting'
import * as fork from 'child_process'
const useTcp = false;



/**
 * ACTIVATION
 *  this method is called when your extension is activate
 *  your extension is activated the very first time the command is executed
 */
var highlight: ASMSemanticHighlightingFeature;
export function activate(context: vscode.ExtensionContext) {
    //debug setup
    context.subscriptions.push(vscode.commands.registerCommand('extension.hlasm-plugin.getProgramName', config => {
		return vscode.window.showInputBox({
			placeHolder: "Please enter the name of a program in the workspace folder",
			value: "pgm"
		});
    }));

    const syncFileEvents = getConfig<boolean>('syncFileEvents', true);
    
    if(useTcp)
    {
        const port = 4746;
        //spawn the server
        fork.spawn(path.join(__dirname, '..', 'bin', 'language_server'), ["-p", port.toString()]);

        //set the tcp communication
        let connectionInfo = {
            port: port,
            host:'localhost'
        };
        var serverOptions: vscodelc.ServerOptions = () => {
            let socket = Net.connect(connectionInfo);
            let result: vscodelc.StreamInfo = {
                writer: socket,
                reader: socket
            };
            return Promise.resolve(result);
        };
    }
    else
    {
        const server: vscodelc.Executable = {
            command: path.join(__dirname, '..', 'bin', 'language_server'),
            args: getConfig<string[]>('arguments')
        };
        var serverOptions: vscodelc.ServerOptions = server;
    }
    const filePattern: string = '**/*'
    const configPattern: string = '**/{' + ['proc_grps.json', 'pgm_conf.json'].join() + '}';
    
    const clientOptions: vscodelc.LanguageClientOptions = {
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

    //client init
    var hlasmpluginClient = new vscodelc.LanguageClient('Hlasmplugin Language Server', serverOptions, clientOptions);
    //asm contribution 
    highlight = new ASMSemanticHighlightingFeature(hlasmpluginClient);
    hlasmpluginClient.registerFeature(highlight);
    console.log('Hlasmplugin Language Server is now active!');

    //first run, set the language if possible
    if (vscode.window.activeTextEditor != undefined)
        setHlasmLanguage(vscode.window.activeTextEditor.document);

    // vscode/theia compatibility temporary fix
    // theia uses monaco commands
    vscode.commands.getCommands().then((result) =>
    {
        if (result.find(command => command == "editor.action.triggerSuggest"))
            completeCommand = "editor.action.triggerSuggest";
        else if (result.find(command => command == "monaco.editor.action.triggerSuggest"))
            completeCommand = "monaco.editor.action.triggerSuggest";
    }).then(() =>
    {
        //give the server some time to start listening
        setTimeout(function() {
            hlasmpluginClient.start();
        }, 2000);
    });
}

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
 * COMMANDS
 * 
 */

class LinePositionsInfo
{
    currentPosition: vscode.Position;
    continuationOffset: number;

    // initializes continuation vars
    constructor(editor: vscode.TextEditor, globalContinuationOffset: boolean = false)
    {
        this.currentPosition = editor.selection.active;
        this.continuationOffset = highlight.getContinuation((globalContinuationOffset) ? -1 : this.currentPosition.line, editor.document.uri.toString());
    }
}

const isInstruction = new RegExp("^(\\S)*\\s+\\S*$");
const isTrigger = new RegExp("^[a-zA-Z\*]+$");
var completeCommand: string;

// find space for line, used for custom type
function findSpace(textLine: vscode.TextLine, length: number, info: LinePositionsInfo)
{
    var spacePosition = info.continuationOffset-1;
    var currentSymbol = textLine.text[spacePosition];
    // go backwards through all the symbols since the continuation and check whether there are enough spaces to compensate for the input
    while (currentSymbol == " " && spacePosition > info.currentPosition.character && length > 0)
    {
        spacePosition--;
        length--;
        currentSymbol = textLine.text[spacePosition];
    }
    return (length > 0) ? null : new vscode.Range(new vscode.Position(textLine.lineNumber, spacePosition), new vscode.Position(textLine.lineNumber, info.continuationOffset-1));
}

function setCursor(editor: vscode.TextEditor, position: vscode.Position)
{
    editor.selection = new vscode.Selection(position, position);
}

function deleteLeft(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, info: LinePositionsInfo)
{
    // size of deleted selection
    var selectionSize = (info.currentPosition.character - editor.selection.anchor.character != 0) ? info.currentPosition.character - editor.selection.anchor.character : 1;
    // position of cursor after delete
    var newCursorPosition = new vscode.Position(info.currentPosition.line, info.currentPosition.character - ((selectionSize > 0 && info.currentPosition.character > 0) ? selectionSize : 0));
    //end of selection
    var endPos = (info.currentPosition.character < editor.selection.anchor.character) ? editor.selection.anchor.character : info.currentPosition.character;
    // there is a continuation and it is after our position, handle it
    if (info.continuationOffset != -1 && endPos < info.continuationOffset && info.currentPosition.character > 0 && editor.selection.isSingleLine) {
        var beforeContinuationChars = new vscode.Range(info.currentPosition.line, info.continuationOffset - Math.abs(selectionSize), info.currentPosition.line, info.continuationOffset);
        edit.insert(beforeContinuationChars.end, " ".repeat(Math.abs(selectionSize)));
    }
    // special case for delete at the beginning of line
    else if (info.currentPosition.character == 0 && info.currentPosition.line > 0) {
        newCursorPosition = new vscode.Position(info.currentPosition.line - 1, editor.document.lineAt(info.currentPosition.line - 1).text.length);
    }
    // delete as default
    edit.delete(new vscode.Range(info.currentPosition, (editor.selection.anchor.character == info.currentPosition.character && editor.selection.anchor.line == info.currentPosition.line) ?
        newCursorPosition :
        new vscode.Position(editor.selection.anchor.line, editor.selection.anchor.character)));
    return newCursorPosition;
}

/**
 * Common logic for both type and paste
 * @param editor 
 * @param edit 
 */
function insertChars(editor: vscode.TextEditor, edit: vscode.TextEditorEdit, text: string)
{
    var info = new LinePositionsInfo(editor);
    // typing with multiple characters selected
    if (info.currentPosition.line != editor.selection.anchor.line || info.currentPosition.character != editor.selection.anchor.character)
    {
        // different cases of insert, depending on the anchor/active positions
        if (editor.selection.anchor.line > editor.selection.active.line)
            edit.insert(editor.selection.active, text);
        else if (editor.selection.anchor.line < editor.selection.active.line)
            edit.insert(editor.selection.anchor, text);
        else
        {
            if (editor.selection.anchor.character < editor.selection.active.character)
                edit.insert(editor.selection.anchor, text);
            else
                edit.insert(editor.selection.active, text);
        }
        
        //simulate delete function for the selected characters
        deleteLeft(editor,edit,info);
    }
    // simple single character insert
    else
        edit.insert(info.currentPosition, text);
    // there is a continuation, prepare space for new characters
    if (info.continuationOffset != -1 && info.currentPosition.character < info.continuationOffset) {
        // find free space in front of it
        var spaceRange = findSpace(editor.document.lineAt(info.currentPosition),text.length, info);
        // if there is, delete it
        if (spaceRange)
            edit.delete(spaceRange);
    } 
}


// overrides should happen only if the user wishes
if (vscode.workspace.getConfiguration().get("hlasmplugin.continuationHandling"))
{
    /**
     * Overriden type to handle continuations
     * Removes space in front of the continuation character to compensate for the newly added
     */
    vscode.commands.registerTextEditorCommand("type", (editor: vscode.TextEditor, edit: vscode.TextEditorEdit, args: {text: string}) => {
        insertChars(editor,edit,args.text);
    });

    /**
     * Overriden paste to handle continuations
     * Removes spaces in front of the continuation character to compensate for the newly added
     */
    vscode.commands.registerTextEditorCommand("paste", (editor: vscode.TextEditor, edit: vscode.TextEditorEdit, args: {text: string}) => {
        insertChars(editor,edit,args.text);
    });

    /**
     * Overriden cut to handle continuations
     * Works similarly to deleteLeft
     */
    vscode.commands.registerTextEditorCommand("cut", (editor: vscode.TextEditor, edit: vscode.TextEditorEdit) => {
        var info = new LinePositionsInfo(editor);
        // do not cut 0 length sequences 
        if (info.currentPosition.line == editor.selection.anchor.line && info.currentPosition.character == editor.selection.anchor.character)
            return;
        var cursorPos = deleteLeft(editor,edit,info);
        // move the cursor if necessary
        if (info.currentPosition.character == info.continuationOffset)
            setCursor(editor,cursorPos);
    });

    /**
     * Overriden default deleteLeft (backspace) to handle continuations
     * Adds extra space(s) in front of the continuation to compensate for the removed characters
     * Works on single line deletions only
     */ 
    vscode.commands.registerTextEditorCommand("deleteLeft", (editor: vscode.TextEditor, edit: vscode.TextEditorEdit) => {
        var info = new LinePositionsInfo(editor);
        var cursorPos = deleteLeft(editor,edit,info);
        // move the cursor if necessary
        if (info.currentPosition.character == info.continuationOffset)
            setCursor(editor,cursorPos);
    });

    /**
     * Overriden default deleteRight (delete) to handle continuations
     * Adds extra space(s) in front of the continuation to compensate for the removed characters
     * Works on single line deletions only
     */ 
    vscode.commands.registerTextEditorCommand("deleteRight", (editor: vscode.TextEditor, edit: vscode.TextEditorEdit) => {
        var info = new LinePositionsInfo(editor);
        // size of deleted selection
        var selectionSize = (info.currentPosition.character - editor.selection.anchor.character != 0) ? info.currentPosition.character - editor.selection.anchor.character : -1;
        // position of cursor after delete
        var newCursorPosition = new vscode.Position(info.currentPosition.line, info.currentPosition.character - ((selectionSize >= -1) ? selectionSize : 0));
        // there is a continuation and it is after our position, handle it
        if (info.continuationOffset != -1 && info.currentPosition.character < info.continuationOffset && editor.selection.isSingleLine) {
            var beforeContinuationChars = new vscode.Range(info.currentPosition.line, info.continuationOffset - Math.abs(selectionSize), info.currentPosition.line, info.continuationOffset);
            edit.insert(beforeContinuationChars.end, " ".repeat(Math.abs(selectionSize)));
        }
        // change the cursor if needed
        if (editor.document.lineAt(info.currentPosition).text.substring(info.currentPosition.character) == "")
            newCursorPosition = new vscode.Position(info.currentPosition.line + 1, 0);
        // delete as default
        edit.delete(new vscode.Range(info.currentPosition, (editor.selection.anchor.character == info.currentPosition.character && editor.selection.anchor.line == info.currentPosition.line) ?
            newCursorPosition :
            new vscode.Position(editor.selection.anchor.line, editor.selection.anchor.character)));
    });

    // insert continuation to the current line
    vscode.commands.registerTextEditorCommand("insertContinuation", (editor: vscode.TextEditor, edit: vscode.TextEditorEdit) => {
        var info = new LinePositionsInfo(editor,true);
        var continueColumn = highlight.getContinueColumn(editor.document.uri.toString());
        // not a continued line, create new continuation
        if (highlight.getContinuation(info.currentPosition.line, editor.document.uri.toString()) == -1) {
            var lastChar = info.currentPosition.character;
            if (info.currentPosition.character < info.continuationOffset) {
                lastChar = editor.document.lineAt(info.currentPosition).text.length;
                lastChar = (lastChar < info.continuationOffset) ? lastChar : info.continuationOffset;
                edit.insert(new vscode.Position(info.currentPosition.line, lastChar), " ".repeat(info.continuationOffset - lastChar));
            }
            var continuationPosition = new vscode.Position(info.currentPosition.line, info.continuationOffset);
            edit.replace(new vscode.Range(continuationPosition, new vscode.Position(info.currentPosition.line, info.continuationOffset + 1)), "X");
            edit.insert(new vscode.Position(info.currentPosition.line, editor.document.lineAt(info.currentPosition).text.length), "\r\n".concat(" ".repeat(continueColumn)));
        }
        // add extra continuation on already continued line
        else
            edit.insert(new vscode.Position(info.currentPosition.line, editor.document.lineAt(info.currentPosition).text.length), "\r\n".concat(" ".repeat(info.continuationOffset).concat("X")));
    });

    // remove continuation from previous line
    vscode.commands.registerTextEditorCommand("removeContinuation", (editor: vscode.TextEditor, edit: vscode.TextEditorEdit) => {
        var info = new LinePositionsInfo(editor,true);
        if (info.currentPosition.line > 0 && highlight.getContinuation(info.currentPosition.line - 1, editor.document.uri.toString()) != -1) {
            var continuationPosition = new vscode.Position(info.currentPosition.line - 1, info.continuationOffset);
            edit.delete(new vscode.Range(new vscode.Position(info.currentPosition.line, editor.document.lineAt(info.currentPosition).text.length), new vscode.Position(info.currentPosition.line - 1, editor.document.lineAt(info.currentPosition.line - 1).text.length)));
            edit.replace(new vscode.Range(continuationPosition, new vscode.Position(continuationPosition.line, info.continuationOffset + 1)), " ");
            setCursor(editor, continuationPosition);
        }
    });

    /**
     * EVENTS
     */

    // when contents of a document change, issue a completion request
    vscode.workspace.onDidChangeTextDocument(event => {
        if ( event.document.languageId != 'hlasm')
            return;

        const editor = vscode.window.activeTextEditor;
        if (event.contentChanges.length == 0 || editor.document.languageId != "hlasm")
            return;
        const change = event.contentChanges[0].text;
        var info = new LinePositionsInfo(editor);
        var currentLine = editor.document.getText(new vscode.Range(new vscode.Position(info.currentPosition.line, 0), info.currentPosition));
        var notContinued = info.currentPosition.line == 0 || highlight.getContinuation(info.currentPosition.line - 1, editor.document.uri.toString()) == -1;
        if ((currentLine != "" && isTrigger.test(change) && isInstruction.test(currentLine) && notContinued && currentLine[0] != "*") || change == "." || change == "&") {
            vscode.commands.executeCommand(completeCommand);
        }
    })
}

vscode.workspace.onDidChangeConfiguration(event =>
{
    if (event.affectsConfiguration("hlasmplugin.continuationHandling"))
        vscode.commands.executeCommand("workbench.action.reloadWindow");
});

// when any visible text editor changes, apply decorations for it
vscode.window.onDidChangeVisibleTextEditors((editors: vscode.TextEditor[]) => {
    for (var i = 0; i < editors.length; i++)
    {
        if (editors[i].document.languageId == 'hlasm')
        {
            highlight.applyDecorations();
            break;
        }
    };
});

const referenceInstructions =new RegExp("( |\\t)+(ICTL|\\*PROCESS|END|COND|IC|ICM|L|LA|LCR|LH|LHI|LM|LNR|LPR|LR|LTR|MVC|MVCL|MVI|ST|STC|STCM|STH|STM|A|AH|AHI|AL|ALR|AR|C|CH|CR|D|DR|M|MH|MHI|MR|S|SH|SL|SLR|SR|CL|CLC|CLCL|CLI|CLM|CLR|N|NC|NI|NR|O|OC|OI|OR|SLA|SLDA|SLDL|SLDA|SLL|SRA|SRDA|SRDL|SRL|TM|X|XC|XI|XR|BAL|BALR|BAS|BASR|BC|BCR|BCT|BCTR|BXH|BXLE|AP|CP|CVB|CVD|DP|ED|EDMK|MP|MVN|MVO|MVZ|PACK|SP|SRP|UNPK|ZAP|CDS|CS|EX|STCK|SVC|TR|TRT|TS|B|J|NOP|BE|BNE|BL|BNL|BH|BHL|BZ|BNZ|BM|BNM|AMODE|CSECT|DC|DS|DSECT|DROP|EJECT|END|EQU|LTORG|ORG|POP|PRINT|PUSH|RMODE|SPACE|USING|TITLE|BP|BNP|BO|BNO|ABEND|CALL|CLOSE|DCB|GET|OPEN|PUT|RETURN|SAVE|STORAGE|COPY|GBLC|GBLB|SETA|SETB|SETC)( |\\t)+");
const macroInstruction =new RegExp("( |\\t)+MACRO( |\\t)*");

// when active editor changes, try to set a language for it
vscode.window.onDidChangeActiveTextEditor((editor: vscode.TextEditor) => 
{
    if (editor != null)
        setHlasmLanguage(editor.document);
})

//automatic detection function
function setHlasmLanguage(document: vscode.TextDocument) {
    if (document.languageId == 'plaintext') {
        var score = 0;
        var lines = 0;
        var lastContinued = false;
        var first = true;
        //iterate line by line
        document.getText().split('\n').forEach(line => {
            // irrelevant line, remove from total count
            if (line != "" && (line[0] != "*" || (line.length > 1 && line[0] == "." && line[1] == "*"))) {
                lines++;
                if (first)
                {
                    //check whether the first line is MACRO and immediately set to hlasm
                    if (macroInstruction.test(line.toUpperCase()))
                    {
                        vscode.languages.setTextDocumentLanguage(document, 'hlasm');
                        return;
                    }
                    else
                        first = false;
                }
                //test if line contains reference instruction
                if ((referenceInstructions.test(line.toUpperCase()) || lastContinued) && line.length <= 80) {
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
        if (score/lines > 0.4)
            vscode.languages.setTextDocumentLanguage(document, 'hlasm');
    }
}
