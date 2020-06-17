\[chap:lang\_server\] The purpose of the Language server is to implement the Language Server Protocol (LSP) and the Debug Adapter Protocol (DAP) and to provide access to the parser library by using them. It has to deserialize and serialize LSP and DAP messages, extract parameters of particular methods and then serve the requests by invoking functionality of parser library.

The language server component is described in the following pages:
1. [[LSP and DAP]]
2. [[Language server overview]]
3. [[IO handling]]
4. [[LSP and DAP server]]
5. [[Request manager]]
