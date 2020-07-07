\[chap:lang\_server\] The purpose of the language server is to implement the Language Server Protocol (LSP) and the Debug Adapter Protocol (DAP) and to provide access to the parser library by using them. It deserializes and serializes LSP and DAP messages, extracts parameters of particular methods and then serves the requests by invoking the functionality of the parser library.

The language server component is described in the following pages:
1. [[LSP and DAP]]
2. [[Language server overview]]
3. [[IO handling]]
4. [[LSP and DAP server]]
5. [[Request manager]]
