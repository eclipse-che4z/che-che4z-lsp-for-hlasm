<div id="header" align="center">

[![GitHub issues](https://img.shields.io/github/issues-raw/eclipse/che-che4z-lsp-for-hlasm)](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues)
[![slack](https://img.shields.io/badge/chat-on%20Slack-blue)](https://join.slack.com/t/che4z/shared_invite/enQtNzk0MzA4NDMzOTIwLWIzMjEwMjJlOGMxNmMyNzQ1NWZlMzkxNmQ3M2VkYWNjMmE0MGQ0MjIyZmY3MTdhZThkZDg3NGNhY2FmZTEwNzQ)
<a href="https://sonarcloud.io/dashboard?id=eclipse_che-che4z-lsp-for-hlasm">
<img src="https://github.com/eclipse/che-che4z-lsp-for-hlasm/raw/master/clients/vscode-hlasmplugin/sonarcloud-black.png" width="94" height="20" href="" />
</a>

</div>

# HLASM Language Support
HLASM Language Support is an extension that supports the High Level Assembler language. It provides code completion, highlighting and navigation features, shows mistakes in the source, and lets you trace how the conditional assembly is evaluated with a modern debugging experience.

This extension is a part of the [Che4z](https://github.com/eclipse/che-che4z) open-source project.

HLASM Language Support is also part of [Code4z](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.code4z-extension-pack), an all-round package that offers a modern experience for mainframe application developers, including [COBOL Language Support](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.cobol-language-support), [Explorer for Endevor](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.explorer-for-endevor), [Zowe Explorer](https://marketplace.visualstudio.com/items?itemName=Zowe.vscode-extension-for-zowe) and [Debugger for Mainframe](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.debugger-for-mainframe) extensions.

## Getting Started

To start using the HLASM Language Support extension, **follow these steps**:

1. Install the extension.
2. In **File** - **Open Folder...**, select the folder where your HLASM project is located.
3. Open your HLASM source code (no file extension is needed) or create a new file.
4. If the extension fails to auto-detect HLASM language, set it manually in the bottom-right corner of the VS Code window.  
   The extension is now enabled on the opened file. If you have macro definitions in separate files or use the COPY instruction, proceed with the steps below to configure the extension to search for external files in the correct directories:
5. After opening the HLASM file, two popups display. Select "Create pgm_conf.json with current program" and "Create empty proc_grps.json".  
   The two configuration files are created in the `.hlasmplugin` subfolder.
6. In the `proc_grps.json` file, fill the `libs` array with paths to folders with macro definitions and COPY files. For example, if you have your macro files in the `ASMMAC/` folder, type the string `"ASMMAC"` into the libs array.

For a full explanation of the configuration, see the [Configuration](#Configuration) section.

## Language Features

The HLASM Language Support extension parses and analyzes all parts of a HLASM program. It resolves all ordinary symbols, variable symbols and checks the validity of most instructions. The extension supports conditional and unconditional branching and can define global and local variable symbols. It can also expand macros and COPY instructions.

## LSP Features
### Highlighting
The HLASM Language Support extension highlights statements with different colors for labels, instructions, operands, remarks and variables. Statements containing instructions that can have operands are highlighted differently to statements that do not expect operands. Code that is skipped by branching AIF, AGO or conditional assembly is not colored.

![](https://github.com/eclipse/che-che4z-lsp-for-hlasm/raw/master/clients/vscode-hlasmplugin/readme_res/highlighting.png)

### Autocomplete
Autocomplete is enabled for the instruction field. While typing, a list of instructions starting with the typed characters displays. Selecting an instruction from the list completes it and inserts the default operands. Variables and sequence symbols are also filled with a value from their scope.

![](https://github.com/eclipse/che-che4z-lsp-for-hlasm/raw/master/clients/vscode-hlasmplugin/readme_res/autocomplete.gif)


### Go To Definition and Find All References
The extension adds the 'go to definition' and 'find all references' functionalities. Use the 'go to definition' functionality to show definitions of variable symbols, ordinary symbols and macros, or open COPY files directly. Use the 'find all references' functionality to show all places where a symbol is used.

![](https://github.com/eclipse/che-che4z-lsp-for-hlasm/raw/master/clients/vscode-hlasmplugin/readme_res/go_to_def.gif)

## Macro Tracer

The macro tracer functionality allows you to track the process of assembling HLASM code. It lets you see step-by-step how macros are expanded and displays values of variable symbols at different points during the assembly process. You can also set breakpoints in problematic sections of your conditional assembly code. 

The macro tracer is not a debugger. It cannot debug running executables, only track the compilation process.

### Configuring the Macro Tracer

1. Open your workspace.
2. In the left sidebar, click the bug icon to open the debugging panel.
3. Click the cog icon in the top left of the screen.  
   A "select environment" prompt displays.
4. Enter **HLASM Macro tracer**.  
   Your workspace is now configured for macro tracing.

### Using the Macro Tracer

To run the macro tracer, open the file that you want to trace. Then press **F5** to open the debugging panel and start the debugging session.

When the tracer stops at a macro or COPY instruction, you can select **step into** to open the macro or COPY file, or **step over** to skip to the next line.

Breakpoints can be set before or during the debugging session.

![](https://github.com/eclipse/che-che4z-lsp-for-hlasm/raw/master/clients/vscode-hlasmplugin/readme_res/tracer.gif)

## External Macro Libraries and COPY Members
The HLASM Language Support extension looks for locally stored members when a macro or COPY instruction is evaluated. The paths of these members are specified in two configuration files in the .hlasmplugin folder of the currently open workspace. Ensure that you configure these files before using macros from separate files or the COPY instruction.

When you open a HLASM file or manually set the HLASM language for a file, you can choose to automatically create these files for the current program.

The structure of the configuration is based on CA Endevor® SCM. `proc_grps.json` defines processor groups by assigning a group name to a list of directories which are searched in the order they are listed. `pgm_conf.json` provides mapping between source files (open code files) and processor groups. It specifies which list of directories is used with which source file. If a relative source file path is specified, it is relative to the current workspace.

Example `proc_grps.json`:

The following example defines two processor groups, GROUP1 and GROUP2, and a list of directories to search for macros and COPY files.

```
{
  "pgroups": [
    {
      "name": "GROUP1",
      "libs": [
        "ASMMAC/",
        "C:/SYS.ASMMAC"
      ]
    },
    {
      "name": "GROUP2",
      "libs": [
        "G2MAC/",
        "C:/SYS.ASMMAC"
      ]
    }
  ]
}
```

Example `pgm_conf.json`:

The following example specifies that GROUP1 is used when working with `source_code.hlasm` and GROUP2 is used when working with `second_file.hlasm`.

```
{
  "pgms": [
    {
      "program": "source_code",
      "pgroup": "GROUP1"
    },
    {
      "program": "second_file",
      "pgroup": "GROUP2"
    },
  ],
  "alwaysRecognize" : ["*.hlasm", "libs/*.asm"]
}
```
If you have the two configuration files configured as above and invoke the MAC1 macro from `source_code.hlasm`, the folder `ASMMAC/` in the current workspace is searched for a file with the exact name "MAC1". If that search is unsuccessful the folder `C:/SYS.ASMMAC` is searched. If that search is unsuccessful an error displays that the macro does not exist.

There is also option `alwaysRecognize` which takes array of wildcards. It allows you to configure two things:
- All files matching these wildcards will be always recognized as HLASM files. 
- If an extension wildcard is defined, all macro and copy files with such extension may be used in the source code. For example, with the extension wildcard `*.hlasm`, a user may add macro `MAC` to his source code even if it is in a file called `Mac.hlasm`.

The program field in `pgm_conf.json` supports wildcards, for example:
```
{
  "pgms": [
    {
      "program": "*",
      "pgroup": "GROUP1"
    }
  ]
}
```
In this example, GROUP1 is used for all open code programs.

## Questions, issues, feature requests, and contributions
- If you have a question about how to accomplish something with the extension, or come across a problem file an issue on [GitHub](https://github.com/eclipse/che-che4z-lsp-for-hlasm)
- Contributions are always welcome! Please see our [GitHub](https://github.com/eclipse/che-che4z-lsp-for-hlasm) repository for more information.
- Any and all feedback is appreciated and welcome!
