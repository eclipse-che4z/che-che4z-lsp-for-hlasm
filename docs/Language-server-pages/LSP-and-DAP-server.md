LSP and DAP Server
------------------

The servers are able to process incoming LSP and DAP requests. They get the messages in the form of already parsed JSONs. Then they extract the name of the requested method with its parameters from the message and call the corresponding method with the parameters encoded in JSON format.

There are two server implementations: `lsp_server` and `dap_server`. Both inherit from an abstract class called `server`. They implement protocol-specific processing of messages — although the protocols are quite similar (both are based on RPC), each protocol has a different initialization and finalization, different message format, etc.

The functionality of servers is divided into `features`. Each feature implements several LSP or DAP methods by unpacking the arguments of the respective method and calling the corresponding parser library function. During initialization, each feature adds its methods to the server’s list of implemented methods. The `lsp_server` uses three features:

-   *Text synchronization feature*, which handles the notifications about the state of open files in the editor.

-   *Workspace folders feature*, which handles the notifications about adding and removing workspaces.

-   *Language feature*, which handles requests about HLASM code information.

The following table shows the list of all implemented LSP methods and the classes where the implementations lie.

| **Component** | **LSP Method Name**                 |
|:--------------|:------------------------------------|
| `lsp_server`  | initialize <br> shutdown <br> exit <br> textDocument/publishDiagnostics|
| Text synchronization feature| textDocument/didOpen <br> textDocument/didChange <br> textDocument/didClose <br> textDocument/semanticHighlighting|
|Workspace folders feature| workspace/didChangeWorkspaceFolders <br> workspace/didChangeWatchedFiles |
|Language feature| textDocument/definition <br> textDocument/references <br> textDocument/hover <br> textDocument/completion|
|

The DAP server uses only one feature — the launch feature, which handles stepping through the code and retrieving information about both variables and stack trace. The following table shows the list of all implemented DAP methods:

| **Class** | **DAP Method Name**              |
|:----------|:---------------------------------|
|`dap_server`| `initialize` <br> `disconnect` <br> `launch`|
|`feature_launch`| `setBreakpoints` <br> `configurationDone` <br> `threads` <br> `stackTrace` <br> `scopes` <br> `next` <br> `stepIn` <br> `variables` <br> `continue` <br> `stopped` <br> `exited` <br> `terminated`|
|

Response With Result
--------------------

According to the LSP and the DAP, the server is required to send messages back to the LSP/DAP client either as responses to requests (e.g. `hover`), notifications (e.g. textDocument/publishDiagnostics notification) or events (e.g. stopped event). Features require reference to an instance of the `response_provider` interface that provides methods `respond` and `notify` for sending messages back to the LSP client. Both LSP and DAP server classes implement the `response_provider` to form protocol-specific JSON with the arguments.

The servers then send the JSON to the LSP/DAP client using the `send_message_provider` interface. At this point, the final complete JSON response is formed. The `send_message_provider` then adds the message header and serializes the JSON using the JSON for Modern C++ library (see [[third party libraries]]).

The only implementation of the `send_message_provider` interface is the [`dispatcher`](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/IO-handling).
