#!/usr/bin/env node

let path  = require('path');
let shell = require('shelljs');

//clean built files
let root = path.dirname(path.dirname(__dirname));
shell.rm('-rf',path.join(root,'node_modules'));
console.log("node_modules removed");
shell.rm('-rf',path.join(root,'lib'));
console.log("lib files removed");
shell.rm('-rf',path.join(root,'language_server.exe'));
shell.rm('-rf',path.join(root,'parser_library.dll'));
shell.rm('-rf',path.join(root,'antlr4-runtime.dll'));
console.log("built files removed");