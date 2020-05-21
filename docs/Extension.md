The frontend of the project is implemented as an extension to a modern IDE instead of creating a completely new GUI. This approach has the advantage of providing a familiar environment and workflow that the developers are used to.

There are several IDEs that currently (natively) support LSP, such as *Eclipse Che* and *Eclipse IDE*, *vim8*, *Visual Studio* and *Visual Studio Code* and many more. Others, for example *IntelliJ*, have plugins which add the support for LSP.

Our IDE choice is *Visual Studio Code*, due to its popularity and lightweight design. Conveniently, *Theia*, a web-based IDE, supports VSCode extensions, therefore our plugin works with *Theia* as well.

Standard LSP Extension
----------------------

The core of the extension is an activation event which starts the plugin for VSCode.

Upon activation, *Language Client* and *[[Language server]]* are started as child processes of VSCode and a pipe is open for their communication. The LSP communication and its features are handled by the *vscodelc* package.

To be independent of pipes, we have added an option to use TCP, which assigns a random free port for TCP communication.

DAP Extension
-------------

[*Macro Tracer*](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/Macro-tracer)  is implemented using DAP, which is also supported out-of-the-box by VSCode. Similarly to LSP TCP support, we dynamically assign a random free port for DAP communication during the activation.

Additional implemented features
-------------------------------

To simplify the work with HLASM in modern editors, several features are added to the extension . These additions are specific for Visual Studio Code (and Theia) and are not a part of the LSP specification.

### Language Detection

The usual workflow with the extension begins with downloading HLASM source codes from mainframe. Typically, these files will not have any file extension and even if they do, they might differ across various products.

To cope with this problem, there are several mechanisms that help the user to recognize the file as HLASM automatically:

- **Macro Detection**  
Each file starting with line *MACRO* (arbitrary number of whitespace before and after) is recognized as HLASM.

- **Configuration Files Detection**  
Every file either defined as a program or as a part of a processor group is recognized as HLASM.

- **Wildcards**  
Configuration file *pgm\_conf.json* contains a field *alwaysRecognize*, which consists of user-defined wildcards. Every file that satisfies at least one of these wildcards is recognized as HLASM.

- **Automatic Language Detection**  
    Whenever a user opens a file, its contents are scanned line by line. If the file has a sufficient ratio of HLASM lines to all lines, it is considered to be HLASM.

    The HLASM line recognition is mostly based on a pre-defined set of most used instructions. If a line correctly uses one of these instructions, it is counted as a HLASM line. Continued line of a HLASM line is also a HLASM line. Moreover, a HLASM line must not exceed 80 characters.

    Comment lines or empty lines are skipped and not counted.

    We tested the detection on 11.000 HLASM files and 9.000 non HLASM files. The best results were observed using 4/10 ratio, with 88% true positive recognition and 95% true negative recognition.

    Because of the indeterminate outcomes, this method is meant to be used as a fall-back in case all previous methods do not suffice.

All detection layers are visualized in the following picture:

<img src="img/lang_detection.svg" alt="Language Detection layers." />

### Continuation Handling

Due to historical reasons, HLASM has a 80 character-per-line limitation. Modern languages do not enforce such restriction and therefore IDEs such as VSCode allow the user to extend their lines freely. This causes 2 major inconveniences.

First of all, the user must add the continuation character on a very specific column manually. Secondly, each time the user types in between continuation character and the instruction/parameters, the continuation character is pushed from its requisite position and needs to be moved back, again manually.

To improve this behavior, the extension offers an option to activate *Continuation Handling*.

The first problem is solved by adding two editor commands *insertContinuation* and *deleteContinuation*, which, when invoked, insert/delete the continuation character on its correct position.

To improve the second problem, the option overrides standard VSCode commands, commonly used when working in editor such as *type*, *deleteLeft*, *deleteRight*, *cut* and *paste*. They offset the continuation character by removing/adding whitespaces in front of it.

### Configuration Prompt

If a workspace contains a HLASM file, but does not have the configuration files set, the user is prompted to create them. The warning message also offers an option to create templates for them.

### HLASM Semantic Highlighting

In case of HLASM, a semantic (server-side) highlighting is desired. The multi-layered nature of the language causes that in quite common scenarios, specific parts of the code can be properly highlighted if and only if some previous part was completely processed (parameters for instructions, skipped code thanks to code generation, defined macros, continuations, etc...).

Based on the open [pull request to the VSCode Language Server](https://github.com/microsoft/vscode-languageserver-node/pull/367/files), we added *semanticHighlighting* as an extra feature of LSP. This feature works in a very similar manner, implementing the LSP interfaces that VSCode provides. It works as a notification from the server to the client, containing ranges inside the document and their respective tokens (e.g. instruction, label, parameter, comment,..).

On top of that, we extended *semanticHighlighting* to *ASMsemanticHighlighting*, which adds the ability to notify the client about a new code layout, specifically begin, continuation and continue columns. These fields can be set in the HLASM code (via ICTL instruction) and are required for the *Continuation Handling* feature to work properly. Our client-server communication is shown in the figure below.

<img src="img/lsp_addition.svg" alt="The addition of semantic highlighting to the LSP communication." />