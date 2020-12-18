The workspace manager component is the only public interface of the parser library. The API design is based on LSP and DAP; most of the API is just LSP/DAP rewritten in C++. The API uses the observer pattern for DAP events and notifications originating in the parser library (e.g. textDocument/publishDiagnostics).

The API methods can be divided into three categories:

-   Editor state and file content synchronization
-   Parsing results presentation
-   Macro tracer

### Editor State and File Content Synchronization

| Method                                            | Description |
|:--------------------------------------------------|:------------|
|`did_open (file name, file content)` <br> `did_change (file name, changes)` <br> `did_close (file name)`|Three methods that are called whenever the user opens a file, changes the contents of an already opened file or closes a file in the editor.|
|`did_change_watched_files (file paths)`|A method that is called when a file from a workspace is changed outsize of the editor|
| `add_workspace (ws name, ws path)` <br> `remove_workspace (ws path` |Methods that are called when the user opens or closes a workspace in the editor|
|

All the methods from the first category are listed in the table above. There are two types of file that need to be synchronized:

-   Files that the user has opened in the editor. Those files are being edited by the user and their content might be different from the files saved in the filesystem.

-   Files that the parser library opens from the hard disk, because they are needed to parse opened files (e.g. a macro that is used by an opened file.)

The parser library can load arbitrary files from the disk, and use their contents until a file is opened in the editor. From that point on, the only source of truth for the contents of the file are the did\_change notifications. Once the file is closed in the editor, the parser library is again allowed to rely on its contents in the filesystem.

### Parsing Results Presentation

| Method                                               | Description |
|:-----------------------------------------------------|:------------|
| `definition(file name, caret position)` |This method gets a position in an opened file. If there is a symbol, the method returns the position that symbol's definition.|
| `references(file name, caret position)` |This method gets a position in an opened file. If there is a symbol, the method returns a list of positions where the symbol is used.|
| `hover(file name, mouse position)`      |This method gets a position in an opened file where the user points with cursor. It returns a list of strings to be shown in a tooltip window.|
| `completion(file name,mouse position, trigger info)`                 |The method gets a position in an opened file and information on how the completion box was triggered (i. e. with what key, automatically/manually). It returns a list of strings suggested for completion at the position.|
|                                                      |             |

All the methods from the second category are listed in the table above. They get the position of the caret or mouse cursor in a file and return information about the place in the code. For example, the method `hover` is called when the user points at some word in the code and waits for a short time. The method returns a string that the editor shows in the tooltip window at the position. Typically, the tooltip shows the type of the variable and its value, if known.

Additionally, the parser library presents its results using the observer pattern. There are two interfaces: highlighting and diagnostics consumer. Each of them uses the `consume` method, which gets updated information as a parameter whenever there is an update. Any potential user of the library (e.g. the language server component) has to implement the interfaces to process the results.

### [[Macro tracer|Macro Tracer]]

The [[macro tracer]] part of the API is also DAP rewritten in C++. There are methods that are called when the user clicks on buttons to control the macro tracer: launch the tracer, step in, step over, continue and stop. Moreover, there are methods that retrieve information about the current state of the traced code: a stack of macro calls and information about compile time variables. See [[Macro tracer]] for a full description.
