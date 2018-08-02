# vscode-hlasmplugin

Provides HLASM language IDE features for VS Code.

## Running debug instance with vscode-hlasmplugin

`vscode-hlasmplugin` provides the features designated by the [Language Server
Protocol](https://github.com/Microsoft/language-server-protocol), such as
code completion, code formatting and goto definition.

### Requirements

* VS Code
* node.js and npm

### Steps

1. Edit the variable `hlasmplugin.path` in user settings of VS Code to point to HlasmLanguageServer binary. 
2. In order to start a development instance of VS code extended with this, run:

```bash
   $ cd /path/to/HlasmPlugin/Clients/vscode-hlasmplugin/
   $ npm install
   $ code .
   $ When VS Code starts, press <F5> (Start debugging).
```