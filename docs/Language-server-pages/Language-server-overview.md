<img src="img/lang_server.svg" alt="Architecture of language server." />

The architecture of the language server component is illustrated in the picture above. It communicates on the standard input/output via LSP with the LSP client and listens on a TCP port to provide DAP support for the macro tracer. The TCP communication is wrapped by the class `tcp_handler`, which abstracts from the complexity of communicating through TCP/IP.

The main purpose of the class [`dispatcher`](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/IO-handling) is to provide abstraction for the lowest level communication, which is shared by LSP and DAP. It reads iostream to parse messages using the JSON for Modern C++ library (see [[third party libraries]]) and stores them in the [`request_manager`](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/Request-manager) as `requests`.

A `request` encapsulates one message that came from the client and is represented only by raw (but parsed) JSON.

`request_manager` stores `requests` in a queue and runs a worker thread that serves the requests one by one. As there is only one instance of `request_manager` running in the language server, it serializes requests from DAP and LSP (which come asynchronously from separate sources) into one queue.

`server` is an abstract class that implements protocol behavior that is common for both DAP and LSP — it implements a Remote Procedure Call. The actual handling of LSP and DAP requests is implemented in `features`. Each `feature` contains implementation of several protocol requests or notifications. The features unwrap the arguments from the JSON and call corresponding parser library methods.

There are two implementations of the abstract `server` class: [`lsp_server`](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/LSP-and-DAP-server) and [`dap_server`](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/LSP-and-DAP-server). They both implement the initialization and finalization of protocol communication, which is a bit different for both protocols and both use features to serve protocol requests.

Example: Hover Request Handling
-------------------------------

<img src="img/hover_sequence.svg" alt="A sequence diagram showing processing of the hover request." />

The image above shows handling of the hover request in the language server. The hover request is sent from the LSP client to the `lsp_server` when the user hovers over the text of a file. The hover request contains the location of the mouse cursor in text, i.e. the name of the file and the number of the line and column where the cursor is. The LSP client then expects a response containing a string (possibly written in markdown language) to be shown in a tooltip box.

The whole process begins with reading from the standard input by the LSP instance of the `dispatcher`. It first reads the header of the message, which contains information about the length of the following JSON. Then it reads the JSON itself and deserializes it using the JSON for Modern C++ library (see [[third party libraries]]). All other components of the language server work only with the parsed representation of the message. The `dispatcher` adds the message to the `request_manager` and returns to reading the next message from the standard input.

The request in the `request_manager` either waits in a queue to be processed, or, if the queue was empty, the worker thread is woken up from sleep using conditional variable. The worker then passes the JSON to the `lsp_server`, which looks at the name of the method written in the message and calls the method “hover” from the language feature.

The hover method unpacks the actual arguments from the JSON and converts any URIs to paths using the cpp-netlib URI library. Then, it calls the hover method from the parser library, which returns a string to be shown in the tooltip next to the hovering mouse. The language feature then wraps the return value back in JSON and calls the `respond` method of its `response_provider` implemented by the `lsp_server`.

The `lsp_server` wraps JSON arguments into a LSP response and uses the `send message provider` implemented by `dispatcher` to send it to the LSP client. The `dispatcher` serializes the JSON, adds the header with the length of the JSON and writes the message to a standard output. Finally, all methods return and the worker thread in `request_manager` looks for another request. If there is none, it goes to sleep.
