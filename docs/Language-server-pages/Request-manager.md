Request Manager
---------------

`request_manager` encapsulates a queue of requests with a worker thread that processes them. There may be up to two [`dispatcher`](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/IO-handling) instances in the language server: one for LSP and one for DAP. Both of them add the requests they parse into one `request_manager`. It is necessary to process the requests one by one, because the parser library cannot process more requests at the same time.

Asynchronous communication is handled by separating the communication into threads:

-   LSP read thread — a thread in which the `dispatcher` reads messages from the standard input.

-   DAP read thread — a thread in which the [`tcp_handler`](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/IO-handling) listens on a localhost port to initiate a DAP session. After accepting the DAP client, the `dispatcher` reads DAP input on this thread too.

-   Worker thread in `request_manager` that processes each request using the [`lsp_server`](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/LSP-and-DAP-server) or the [`dap_server`](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/LSP-and-DAP-server) and ultimately the parser library.

The threads are synchronized in two ways: First, there is a mutex that prevents the LSP and the DAP threads from adding to the request queue simultaneously. Second, there is a conditional variable to control the worker thread.

`request_manager` additionally incorporates a mechanism for invalidating requests that have been obsoleted by new requests. The obsoleting of requests is done by a cancellation token. It is shared between the parser library and the `request_manager`. When set to true, the results of current request or notification are no longer needed, the parser library stops all parsing and return as soon as possible.

When a new request arrives, all previous requests (including the currently processed one) that concern the same file are invalidated. However, they cannot be simply removed from the queue. They still have to be processed as they may carry information that must not be discarded (e.g. changes to contents of a file). The parser library processes the request but does not reparse any source files.

### Example of request invalidating

For example, when a user starts changing a file, every character he writes is passed to the language server as a textDocument/didChange notification. Each such notification is processed in two stages:

1.  The parser library changes the internal representation of the text document.

2.  The parser library starts the parsing of the file to update diagnostics and highlighting. This may take some time.

When more didChange notifications come in succession, their first parts must be executed with all the notifications to keep the internal representation of the file updated. However, the user is interested only in diagnostics and semantic highlighting for the current state of the text, so we need to parse the file only once — after the last notification.
