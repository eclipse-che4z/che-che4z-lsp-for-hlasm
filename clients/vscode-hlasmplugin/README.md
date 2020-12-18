<div id="header" align="center">

[![GitHub issues](https://img.shields.io/github/issues-raw/eclipse/che-che4z-lsp-for-hlasm)](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues)
[![slack](https://img.shields.io/badge/chat-on%20Slack-blue)](https://communityinviter.com/apps/che4z/code4z)
<a href="https://sonarcloud.io/dashboard?id=eclipse_che-che4z-lsp-for-hlasm">
<img src="https://github.com/eclipse/che-che4z-lsp-for-hlasm/raw/master/clients/vscode-hlasmplugin/sonarcloud-black.png" width="94" height="20" href="" />
</a>

</div>

# HLASM Language Support
HLASM Language Support is an extension that supports the High Level Assembler language. It provides code completion, highlighting and navigation features, shows mistakes in the source, and lets you trace how the conditional assembly is evaluated with a modern debugging experience.

HLASM Language Support is also part of [Code4z](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.code4z-extension-pack), an all-round package that offers a modern experience for mainframe application developers, including [COBOL Language Support](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.cobol-language-support), [Explorer for Endevor](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.explorer-for-endevor), [Zowe Explorer](https://marketplace.visualstudio.com/items?itemName=Zowe.vscode-extension-for-zowe) and [Debugger for Mainframe](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.debugger-for-mainframe) extensions.

## Getting Started

### Enabling the Extension

Follow these steps to open a HLASM project:

1. In _File_ -> _Open Folder..._, select the folder with the HLASM sources. <!-- (An example workspace is provided in the folder `example_workspace`.) Uncomment once PR#44 is merged-->
2. Open any HLASM source file (note that HLASM does not have a standard filename extension) or create a new file.
3. If the auto-detection of HLASM language does not recognize the file, set it manually in the bottom-right corner of the VS Code window.  
4. The extension is now enabled on the open file. If you have macro definitions in separate files or use the COPY instruction, you need to set up a workspace.

### Setting Up a Multi-File Project Environment

The HLASM COPY instruction copies the source code from various external files, as driven by HLASM evaluation. The source code interpreter in the HLASM Extension needs to be set up correctly to be able to find the same files as the HLASM assembler program. 

To do this, set up two configuration files — `proc_grps.json` and `pgm_conf.json`. Follow these steps:

1. After you open a HLASM file for the first time, two pop-ups display. Select _Create pgm_conf.json with current program_ and _Create empty proc_grps.json_. 
   The two configuration files are then created with default values. They are stored in the `.hlasmplugin` subfolder.
2. Navigate to the `proc_grps.json` file. This is the entry point where you can specify paths to macro definitions and COPY files. 
3. Fill the `libs` array with the corresponding paths. For example, if you have your macro files in the `ASMMAC/` folder, add the string `"ASMMAC"` into the libs array.

Follow the section *External Macro Libraries and COPY Members* below for more detailed instructions on configuring the environment.

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
2. In the left sidebar, click the bug icon to open the debugging panel (Ctrl + Shift + D).
3. Select `create a launch.json file`.  
   A "select environment" prompt displays.
4. Enter *HLASM Macro tracer*.  
   Your workspace is now configured for macro tracing.

### Using the Macro Tracer

To run the macro tracer, open the file that you want to trace. Then press **F5** to open the debugging panel and start the debugging session.

When the tracer stops at a macro or COPY instruction, you can select **step into** to open the macro or COPY file, or **step over** to skip to the next line.

Breakpoints can be set before or during the debugging session.

![](https://github.com/eclipse/che-che4z-lsp-for-hlasm/raw/master/clients/vscode-hlasmplugin/readme_res/tracer.gif)

## Configuration

### External Macro Libraries and COPY Members
The HLASM Language Support extension looks for locally stored members when a macro or COPY instruction is evaluated. The paths of these members are specified in two configuration files in the `.hlasmplugin` folder of the currently open workspace:

- `proc_grps.json` defines _processor groups_ by assigning a group name to a list of directories. Hence, the group name serves as a unique identifier of a set of HLASM libraries defined by a list of directories.

- `pgm_conf.json` provides a mapping between _programs_ (open-code files) and processor groups. It specifies which list of directories is used with which source file. If a relative source file path is specified, it is relative to the current workspace.

To use a predefined set of macro and copy members, follow these steps: 
1. Specify any number of library directories to search for macros and COPY files in `proc_grps.json`. These directories are searched in order they are listed. 
2. Name the group of directories with an identifier.
   You have created a new processor group.
3. Use the identifier of the new processor group with the name of your source code file in `pgm_conf.json` to assign the library members to the program.

The structure of the configuration is based on CA Endevor® SCM. Ensure that you configure these files before using macros from separate files or the COPY instruction.
When you open a HLASM file or manually set the HLASM language for a file, you can choose to automatically create these files for the current program.

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

The following example specifies that GROUP1 is used when working with `source_code` and GROUP2 is used when working with `second_file`.

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
  ]
}
```
If you have the two configuration files configured as above and invoke the MAC1 macro from `source_code`, the folder `ASMMAC/` in the current workspace is searched for a file with the exact name "MAC1". If that search is unsuccessful the folder `C:/SYS.ASMMAC` is searched. If that search is unsuccessful an error displays that the macro does not exist.

The program name in `pgm_conf.json` can be wildcarded, as in the following example:

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

### File Extensions

`pgm_conf.json` includes the optional parameter `alwaysRecognize` in which you can specify an array of wildcards. 
- All files matching these wildcards are automatically recognized as HLASM files.
- If an extension wildcard is defined, all macro and copy files with this extension can be used in the source code. 

For example, with the extension wildcard `*.hlasm`, a user can add the macro `MAC` to his source code even if it is in a file called `MAC.hlasm`. Additionally, all files with the extension `.hlasm` are automatically recognised as HLASM files.

The following example of `pgm_conf.json` specifies that the processor group `GROUP1` is assigned to both `source_code` and `source_code.hlasm`. Also, macro and copy files in the `lib` directory are referenced and correctly recognized in the program without the `.asm` extension.

```
{
  "pgms": [
    {
      "program": "source_code",
      "pgroup": "GROUP1"
    }
  ],
  "alwaysRecognize" : ["*.hlasm", "libs/*.asm"]
}
```

### Suppression of Diagnostics

For files, that use macros extensively, but do not have the definitions available, it is very probable that diagnostics reported by HLASM Language support will not be helpful. For those cases, there is the setting `diagnosticsSuppressLimit`, which can be set either in editor settings, or in `pgm_conf.json`. For files with no processor group configuration in `pgm_conf.json`, all the diagnostics will be suppressed, if there is too many of them (more than the configured limit).

```
{
  "pgms": [
    {
      "program": "source_code",
      "pgroup": "GROUP1"
    }
  ],
  "diagnosticsSuppressLimit" : 15
}
```
With the `pgm_conf.json` above, the `source_code` file has configuration, so all discovered diagnostics will be always shown. However, if you open another file and do not assign a processor group to it, its diagnostcs will not be shown, if there are more than 15 of them.



## Questions, issues, feature requests, and contributions
- If you have a question about how to accomplish something with the extension, or come across a problem file an issue on [GitHub](https://github.com/eclipse/che-che4z-lsp-for-hlasm)
- Contributions are always welcome! Please see our [GitHub](https://github.com/eclipse/che-che4z-lsp-for-hlasm) repository for more information.
- Any and all feedback is appreciated and welcome!
