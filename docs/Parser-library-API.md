First of all, the workspace manager component is the only public interface of the parser library. The API design is based on LSP and DAP, most of the API is just LSP/DAP rewritten in C++. The API uses the observer pattern for DAP events and notifications originating in parser library (e.g. textDocument/publishDiagnostics).

The API methods can be divided into three categories:

-   Editor state and file content synchronization
-   Parsing results presentation
-   Macro tracer

### Editor state and file content synchronization

| Method                                            | Description |
|:--------------------------------------------------|:------------|
|`did_open (file name, file content)` <br> `did_change (file name, changes)` <br> `did_close (file name)`|Three methods that are called whenever the user opens a file, changes contents of an already opened file or closes a file in the editor.|
|`did_change_watched_files (file paths)`|Method, that is called when a file from a workspace has been changed outsize of the editor|
| `add_workspace (ws name, ws path)` <br> `remove_workspace (ws path` |Methods that are called when the user opens or closes a workspace in the editor|
|

All the methods from the first category are listed in the table above. There are two types of files that need to be synchronised:

-   Files, that the user has opened in the editor. Those files are being edited by the user and their content may be different from the files actually saved in the filesystem.

-   Files, that the parser library opens from the hard disk, because they are needed to parse opened files (e.g. a macro that is used by an opened file)

So the parser library is allowed to load arbitrary files from the disk, and use its contents until such file is opened in the editor. From that point on, the only source of truth for the contents of the file are the did\_change notifications. Once the file is closed in the editor, the parser library is again allowed to rely on its contents in the filesystem.

### Parsing results presentation

| Method                                               | Description |
|:-----------------------------------------------------|:------------|
| `definition(file name, caret position)` |The method gets a position in an opened file. If there is a symbol, the method returns position of definition of that symbol|
| `references(file name, caret position)` |The method gets a position in an opened file. If there is a symbol, the method returns list of positions where the symbol is used|
| `hover(file name, mouse position)`      |The method gets a position in an opened file where the user points with cursor. Returns list of strings to be shown in a tooltip window|
| `completion(file name,mouse position, trigger info)`                 |The method gets a position in an opened file and how the completion box was triggered (i. e. with what key, automatically/manually). Returns list of strings suggested for completion at the position|
|                                                      |             |

All the methods from the second category are listed in the table above. They get position of caret or mouse cursor in a file and are expected to return information about the place in the code. For example, method `hover` is called when the user points at some word in the code and waits for a short time. The method returns a string that the editor shows in the tooltip window at the position. Typically, the tooltip would show type of the variable and its value, if known.

Additionally, the parser library presents its results using the observer pattern. There are two interfaces: highligting and diagnostics consumer. Each of them has method `consume` that gets updated information as parameter whenever there is an update. Any potential user of the library (e.g. the language server component) just has to implement the interfaces to process the results.

### [[Macro tracer]]

The [[macro tracer]] part of the API is again just DAP rewritten in C++. There are methods that are called when the user clicks on buttons to control the macro tracer: launch the tracer, step in, step over, continue and stop. Moreover, there are methods that retrieve information about current state of traced code: stack of macro calls and information about compile time variables. See [[Macro tracer]] for full description.

