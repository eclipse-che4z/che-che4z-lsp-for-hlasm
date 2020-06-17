The IBM High Level Assembler Language (HLASM) is still actively used commercially, even though it is a relatively old language. Its roots go back to the 1970s, when IBM made their first mainframes. Since then, the IBM assembler has been revised several times — the last version (which is the concern of this project) was released in 1992. Although it is hard to believe, a lot of the software that has been written in the language over the years is still actively used and maintained, mainly because of the conservative mainframe users and IBM’s vendor lock-in.

Today, HLASM developers are forced to code in archaic terminals directly on the mainframe. Therefore, they spend a lot of time navigating around the code and the environment. For example, solely due to the fact that the user needs to navigate through plenty of terminal screens it takes around a minute just to get to a screen where it is possible to make a change in a file and recompile. For developers, it would be extremely useful to have an IDE plugin that would minimize contact with the mainframe terminal, could analyze the HLASM program, check its validity and make the code clearer by syntax highlighting.

We introduce such plugin for Visual Studio Code, which is one of the most popular code editors nowadays. It improves HLASM programming experience, so that it can be compared to coding in modern programming languages, by providing instant code validity checks, advanced highlighting, code analysis, and all the functionality that a programmer currently takes for granted when writing code.

The most significant properties and features of the plugin are:

-   It is capable of interpreting and tracing a large subset of HLASM code-generating instructions.
-   It contains a list of all built-in instructions that is used to validate the generated code.
-   *[[Macro tracer]]* gives a possibility to trace the compilation of a HLASM source code step-by-step in a way similar to common debugging.
-   It implements [[LSP and DAP]] protocols, providing interface that can be easily connected to numerous modern code editors.
-   It was successfully used on a production HLASM codebase with over 15 million lines of code.

The plugin is available on the [Visual Studio Code Marketplace](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.hlasm-language-support).

This wiki serves as an in-depth documentation for anyone who would like to understand the implementation of the project and the reasons behind it. It is advised that the potential contributors to the project read this documentation first.

User documentation is available on the [Visual Studio Code Marketplace](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.hlasm-language-support).

Organisation of this wiki
-----------------------------

First of all, in [[HLASM overview]], we briefly explain the basics of HLASM needed to comprehend the workflow of this language. In [[Architecture overview]], we provide an overview of the project’s architecture, naming the most important components and indicating their relations. Then, we describe these components in separate chapters in further detail. In [[Language server]], we state the responsibilities of the language server as the communication provider between the extension client and the parsing library. The [[workspace manager]] is the entry point to the parsing library used by the [[language server]]. The purpose of its sub-components is to handle file management, dependency resolution and parsing. The core of the processing of a HLASM file is implemented inside the [[analyzer]]. The project also provides macro tracing through the standard debugging procedure and it is fully explained in \[[macro tracer]].The last mentioned component is the [[VSCode extension|Extension]], which communicates with the [[language server]] and provides IDE features to the user. In [[Build instructions]], we provide a guide how to build this project.