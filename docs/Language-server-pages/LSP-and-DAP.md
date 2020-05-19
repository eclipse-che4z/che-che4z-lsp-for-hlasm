Language Server Protocol
------------------------

[Language Server Protocol](https://microsoft.github.io/language-server-protocol/) is used to extend code editors with support for additional programming languages. LSP defines 2 communicating entities: a client and a server. The LSP client is editor-specific and wraps interaction with the user. The LSP server is language-specific and provides information about the source code.

The main purpose of the LSP is to allow the language server to provide language-specific response to various user interactions with the code editor. Messages that flow through LSP can be divided into three categories:

-   **Parsing results presentation** Messages from the first category allow the language server to send results of source code analysis to the LSP client. The editor is then able to show them to the user. For example, when the user clicks on a symbol in HLASM code and then uses the ‘Go to definition’ function, the LSP client sends a request to the language server with the name of currently open file and current location in the file. The server is then expected to send back the location of the definition, so the editor can present it to the user (e.g. the editor moves the caret to the definition location). All such messages are listed in the following table:

    | Message                 | Description |
    |:------------------------|:------------|
    | textDocument/definition |The client sends a position in an open file. The server responds with a position of a definition of a symbol at that position.|
    | textDocument/references |The client sends a position in an open file. If there is a symbol, the server responds with a list of positions where the symbol is used.|
    | textDocument/hover      |The client sends a position in an open file where the user is pointing with the cursor. The server responds with a string to be shown in a tooltip window.|
    | textDocument/completion |The client sends a position in an open file and how a completion box was triggered (i.e. with what key, automatically/manually). The server responds with a list of strings suggested for completion at the position.|
    |textDocument/publishDiagnostics|The server sends diagnostics to the client. A diagnostic represents a problem with the source code, e.g. compilation errors and warnings.|


-   **Editor state and file content synchronization** Messages from the second category flow mainly from the client to the server and ensure that the server has enough information to correctly analyze source code. All such messages can be found in the following table:
    | Message                   | Description |
    |:--------------------------|:------------|
    | textDocument/didOpen <br> textDocument/didChange <br> textDocument/didClose|The server is notified whenever the user opens a file, changes contents of an already open file or closes a file in the editor.|
    | workspace/didChangeWatchedFiles|The client notifies the server when a watched file is changed outside of the editor. Watched files selector is defined when the client is started (in the extension component).|
    | workspace/didChangeWorkspaceFolders|The client notifies the server that the user has opened or closed a workspace.|


-   **LSP initialization and finalization** Lastly, there are several messages that handle protocol initialization and finalization.




LSP is based on [JSON RPC](https://www.jsonrpc.org/specification). There are two types of interaction in JSON RPC: requests and notifications. Both of them carry the information to invoke a method on the recipient side — name of the method and its arguments. The difference between the two is that each request requires a response containing the result of the method, whereas the notifications do not.

The LSP uses the JSON RPC specification and further specifies how messages are transferred and defines methods, their arguments, responses and semantics. A raw message sent from the client to the server is shown in the following snippet:

    Content-Length: 123\r\n
    \r\n
    {"jsonrpc":"2.0","method":"textDocument/didClose","params":{"textDocument":
    {"uri":"file:/c%3A/Users/admin/Documents/source.hlasm"}}}
    	

The raw messages have HTTP-like headers. The only mandatory header is `Content-Length`, which tells the recipient the length of the following message. The JSON itself is sent after the header.

Inside the JSON, there is a name of the method to be invoked and parameters to pass to the method. In this case, the client is sending a notification that file `C:/Users/admin/Documents/source.hlasm` was closed in the editor by the user. As it is a notification, there must not be any response.

On top of this basic protocol, LSP defines methods and their semantics to cover common functionality that users expect when programming in an editor. List of all methods implemented in the language server can be found in [[LSP and DAP server]].

DAP
---

[Debug Adapter Protocol](https://microsoft.github.io/debug-adapter-protocol/) is used to extend code editors with debugging support for additional programming languages. We use it to provide the user with the ability to trace how the HLASM compiler processes source code step by step. The user can see the values of compile-time variables and follow the expansion of macros in debug-like experience.

The communication in DAP is between an editor or an IDE and a debugger. The editor notifies the debugger about the user actions, e.g. when a breakpoint is set or when the user uses step in/step over buttons. The debugger informs the editor about the state of the debugged application, for example when the debugger stopped because it hit a breakpoint. While it is stopped, the debugger sends information about program stack, variables valid in current debugger scope and its values.

DAP is very similar to LSP. Although the ideas behind DAP are nearly the same, DAP is not based on the JSON RPC. Instead, DAP specifies its own implementation of remote procedure call, still using JSON as the basic carrier of the messages. DAP has requests and events — requests always go from the client to the server and require response. Events are the same as the notifications from JSON RPC that are sent from the server to the client. The similarity allows our language server component to share a lot of code between the implementations of the protocols.
