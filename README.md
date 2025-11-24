<div id="header" align="center">

[![GitHub issues](https://img.shields.io/github/issues-raw/eclipse-che4z/che-che4z-lsp-for-hlasm)](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues)
[![slack](https://img.shields.io/badge/chat-on%20Slack-blue)](https://join.slack.com/t/che4z/shared_invite/zt-37ewynplx-wCoabaIDxN6Ofm4_XBinZA)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=eclipse-che4z_che-che4z-lsp-for-hlasm&metric=alert_status)](https://sonarcloud.io/dashboard?id=eclipse-che4z_che-che4z-lsp-for-hlasm)

</div>

# HLASM Language Support
HLASM Language Support is an IDE extension that supports the High Level Assembler language. It provides code completion, highlighting and navigation features, shows mistakes in the source, retrieves  dependencies from mainframe data sets and from Endevor, and enables you to trace how the conditional assembly is evaluated with a modern debugging experience.

HLASM Language Support is part of the Che4z open-source project. Feel free to contribute at our [GitHub repository](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/). For project documentation, see the [wiki](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/wiki/). 

HLASM Language Support is also part of [Code4z](https://techdocs.broadcom.com/code4z), an all-round VS Code extension package that offers a modern experience for mainframe application developers, including tools for language support, data editing, testing, and source code management. For an interactive overview of Code4z, see the [Code4z Developer Cockpit](https://mainframe.broadcom.com/code4z-developer-cockpit).

## Integration with Explorer for Endevor and Zowe Explorer

Integrate HLASM Language Support with [Explorer for Endevor](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.explorer-for-endevor) to retrieve your HLASM source code from Endevor locations and edit it in VS Code. HLASM Language Support automatically retrieve dependencies associated with Endevor elements written in HLASM. We also recommend that you use HLASM Language Support with [Zowe Explorer](https://marketplace.visualstudio.com/items?itemName=Zowe.vscode-extension-for-zowe) to open your High Level Assembler programs stored on mainframe data sets.

HLASM Language Support, Explorer for Endevor and Zowe Explorer are all available as part of the [Code4z](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.code4z-extension-pack) package.

<a href="https://www.openmainframeproject.org/all-projects/zowe/conformance"><img alt="This extension is Zowe v3 conformant" src="https://artwork.openmainframeproject.org/other/zowe-conformant/zowev3/explorer-vs-code/color/zowe-conformant-zowev3-explorer-vs-code-color.png" width=20% height=20% /></a>

## Prerequisites

To enable automatic dependency retrieval from Endevor, install the following:
- Endevor REST API version 2.16 or higher (PTF LU09053).
- Explorer for Endevor version 1.7.0 or higher.

## Compatibility

HLASM Language Support is supported on Visual Studio Code, GitHub Codespaces and Visual Studio Code for the Web. The language server component can be also integrated with other LSP compatible editors (e.g. Neovim).

#### Restriction

Bulk and on-demand downloading of dependencies via FTP is not available in the Web extension environment.

## Getting Started

### Enabling the Extension

Follow these steps to open a HLASM project:

1. In **File** -> **Open Folder...** select the folder with the HLASM sources. You can download and open an <nobr>`example workspace`</nobr> from our [GitHub repository](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/tree/master/example_workspace).
2. Open any HLASM source file (note that HLASM does not have a standard filename extension) or create a new file.
3. If the auto-detection of HLASM language does not recognize the file, set it manually in the bottom-right corner of the VS Code window.  
4. The extension is now enabled on the open file. If you have macro definitions in separate files or use the COPY instruction, you must set up a workspace.

### Setting Up a Multi-File Project Environment

External files are usually accessed during HLASM evaluation (e.g. when the HLASM COPY instruction is used, or when macros are defined in external libraries). The source code interpreter in the HLASM Language Support extension must be set up correctly to be able to find the same files as the HLASM assembler program. 

To do this, set up two configuration files — `proc_grps.json` and `pgm_conf.json`. Follow these steps:

1. After you open a HLASM file for the first time, two pop-ups display. Select **Create empty proc_grps.json** and **Create pgm_conf.json with this file**. 
   Two configuration files are then created with default values. They are stored in the `.hlasmplugin` subfolder.
2. Navigate to the `proc_grps.json` file. This is the entry point where you can specify paths to macro definitions and COPY files. 
3. Fill the `libs` array with the corresponding paths. For example, if you have your macro files in the `ASMMAC/` folder, add the string `"ASMMAC"` into the libs array.

Follow the section [External Macro Libraries and COPY Members](#External-Macro-Libraries-and-COPY-Members) below for more detailed instructions on configuring the environment.

The `pgm_conf.json` file can be provided implicitly by another product that supports integration with HLASM Language Support (e.g. Endevor Bridge for Git).

You can also specify your processor group configuration in the Visual Studio Code extension settings in the `hlasm.proc_grps` and `hlasm.pgm_conf` keys. When `proc_grps.json` or `pgm_conf.json` files are present in the workspace, they take precedence over any configuration that is specified in the extension settings.

## Language Features

The HLASM Language Support extension parses and analyzes all parts of a HLASM program including the listing. It resolves all ordinary symbols, variable symbols and checks the validity of most instructions. The extension supports conditional and unconditional branching and can define global and local variable symbols. It can also expand macros and COPY instructions.

## LSP Features
### Highlighting
The HLASM Language Support extension highlights statements with different colors for labels, instructions, operands, remarks and variables. Statements containing instructions that can have operands are highlighted differently to statements that do not expect operands. Code that is skipped by branching `AIF`, `AGO` or conditional assembly is not colored.

![](/clients/vscode-hlasmplugin/readme_res/highlighting.png)

### Autocomplete
Autocomplete is enabled for the instruction field. While typing, a list of instructions starting with the typed characters displays. Selecting an instruction from the list completes it and inserts the default operands. Variables and sequence symbols are also filled with a value from their scope.

![](/clients/vscode-hlasmplugin/readme_res/autocomplete.gif)

### Go To Definition and Find All References
The extension adds the 'go to definition' and 'find all references' functionalities. Use the 'go to definition' functionality to show definitions of variable symbols, ordinary symbols and macros, or open COPY files directly. Use the 'find all references' functionality to show all places where a symbol is used.

![](/clients/vscode-hlasmplugin/readme_res/go_to_def.gif)

### Branch Indicators
In the HLASM Language Support extension settings, enable the **Hlasm: Show Branch Indicators** option to add branch indicators to the source code view. Branch indicators display as arrows to the left of instructions and indicate whether the instructions branch up or down. If the branching direction cannot be determined, a right arrow displays.

## Macro Tracer

The macro tracer functionality allows you to track the process of assembling HLASM code. It lets you see step-by-step how macros are expanded and displays values of variable symbols at different points during the assembly process. You can also set breakpoints in problematic sections of your conditional assembly code. 

The macro tracer is not a debugger. It cannot debug running executables, it only tracks the compilation process.

### Configuring the Macro Tracer

1. Open your workspace.
2. In the left sidebar, click the bug icon to open the debugging panel (`Ctrl + Shift + D`).
3. Select `create a launch.json file`.  
   The file `launch.json` opens with a pre-filled configuration.  
   Your workspace is now configured for macro tracing.   

### Using the Macro Tracer

To run the macro tracer, open the file that you want to trace. Then press **`F5`** to open the debugging panel and start the debugging session.

When the tracer stops at a macro or COPY instruction, you can select **step into** to open the macro or COPY file, or **step over** to skip to the next line.

Breakpoints can be set before or during the debugging session.

![](/clients/vscode-hlasmplugin/readme_res/tracer.gif)

## Configuration

### External Macro Libraries and COPY Members
The HLASM Language Support extension looks for locally stored members when a macro call or COPY instruction is evaluated. The paths of these members are specified in two configuration files in the `.hlasmplugin` folder of the currently open workspace:

- `proc_grps.json` defines _processor groups_ by assigning a group name to a list of directories. Hence, the group name serves as a unique identifier of a set of HLASM libraries that are defined by a list of directories (some of which can be optional). Additionally, some assembler options can be specified in `asm_options` sections (`SYSPARM`, `SYSTEM_ID` and others).

- `pgm_conf.json` provides a mapping between programs (open-code files) and processor groups.
    - **Note:** If you use HLASM Language Support together with Endevor Bridge for Git, the Endevor Bridge for Git configuration file `.bridge.json` can also be used to link programs and processor groups instead of `pgm_conf.json`. For more information, see the "Other Configuration Files" section below. 

To use a predefined set of macro and copy members, follow these steps: 
1. Specify any number of library directories or remote data sets to search for macros and COPY files in `proc_grps.json`. These directories and data sets are searched in order that they are listed.
2. Name the group of directories with an identifier.
   You have created a new processor group.
3. Use the identifier of the new processor group with the name of your source code file in `pgm_conf.json` to assign the library members to the program.

Relative paths that you specify in `proc_grps.json` (for libraries) or in `pgm_conf.json` (for programs) are resolved with respect to the current workspace. If you set the option `prefer_alternate_root` to `true`, relative paths are resolved with respect to the directory provided by an integration (e.g. Endevor Bridge for Git integration).

The structure of the configuration is based on Endevor®. Ensure that you configure these files before you use macros or the COPY instruction.

Visual Studio Code workspace variables can be referenced in both configuration files using the standard syntax `${config:variable_name}`.

### Example `proc_grps.json`:

The following example defines two processor groups, GROUP1 and GROUP2, and a list of directories to search for macros and COPY files, it also defines the _SYSPARM_ assembler parameter and values for two external functions for GROUP1. Additionally, if the library `MACLIB/` does not exist in the workspace, the plugin does not report it as an error.

The `SYS1.MACLIB` data set is accessed via an FTP client and required members are downloaded.

Wildcards can be used to locate libraries and/or programs as is shown in the path mask `C:/common/**/maclib` below. The following wildcards are supported:
- `?` -  Matches a single character but not a directory separator 
- `*` -  Matches 0 characters, 1 characters or a continuous sequence of characters but not a directory separator
- `**` - Matches 0 characters, 1 character or a continuous sequence of characters including directory separators
Wildcards can be used only when specifying local libraries, not remote data sets.

The order of libraries that are selected by a path mask is arbitrary. We therefore recommend that macro names within these libraries are unique.

```
{
  "pgroups": [
    {
      "name": "GROUP1",
      "libs": [
        "ASMMAC/",
        {
          "path": "MACLIB/",
          "optional": true
        },
        "C:/SYS.ASMMAC",
        "C:/common/**/maclib",
        {
          "dataset": "SYS1.MACLIB"
        }
      ],
      "asm_options": {
        "SYSPARM": "ZOS210"
      },
      "external_functions": {
        "FUNC1": 1,
        "FUNC2": "STRING"
      }
    },
    {
      "name": "GROUP2",
      "libs": [
        "G2MAC/",
        "C:/SYS.ASMMAC",
        "C:/common/**/maclib"
      ]
    }
  ]
}
```

### Example `pgm_conf.json`:

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
    }
  ]
}
```

When you have both `proc_grps.json` and `pgm_conf.json` configured as above and you invoke the MAC1 macro from the `source_code`, the folder `ASMMAC/` in the current workspace is searched for a file with the name "MAC1". If "MAC1" file isn't found, the folder `C:/SYS.ASMMAC` is searched. If even this search is unsuccessful, an error saying that the macro does not exist is displayed.

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

Assembler options defined by the processor group can be overridden in the `pgm_conf.json` file as shown in the following example:

```
{
  "pgms": [
    {
      "program": "source_code",
      "pgroup": "GROUP1",
      "asm_options":
      {
        "SYSPARM": "SYSPARM override value"
      }
    }
  ]
}
```

### Other Configuration Files

#### `.bridge.json` Configuration File
If you use Endevor Bridge for Git, your workspace might already have `.bridge.json` configuration files which contain program to processor group mappings. In this case, you do not need to create a separate `pgm_conf.json` file. However, `proc_grps.json` is still required to enable successful mapping between programs specified in `.bridge.json` and processor groups defined in `proc_grps.json`.

#### Example of `.bridge.json`:
In this `.bridge.json` file, the program `source_code` is mapped to a processor group `GROUP1` and all other existing programs are mapped to `GROUP2` by default.

```
{
  "elements": {
    "source_code": {
      "processorGroup": "GROUP1"
    }
  },
  "defaultProcessorGroup": "GROUP2",
  "fileExtension": ""
}
```

#### Configuration Lookup Precedence

If you use multiple program to processor group mapping files (such as `pgm_conf.json` and `.bridge.json`) or wildcards in your program names, the same program might be specified more than once. In this case, the program to processor group mapping is determined by the following precedence hierarchy:

1. Processor group bound to a specific program name specified in `pgm_conf.json`
2. Processor group bound to a wildcarded program name specified in `pgm_conf.json`
3. Processor group bound to a specific program name specified in `.bridge.json`
4. Default processor group specified in `.bridge.json`

### File Extensions

The `alwaysRecognize` option in `pgm_conf.json` has been deprecated in favor of the standard VSCode user and workspace level setting `file.associations`.

`proc_grps.json` can include an optional parameter `macro_extensions` which contains a list of extensions that are used to identify files with macro definitions.
The options can be specified both at the top level of the file, which provides the default list for all libraries in all process groups, and at the level of individual library definitions, which override the default from the top level.
If the `macro_extensions` parameter is not provided or is empty the language server ignores file extensions. Warning messages are produced when conflicting names are detected.

For example, with the extension `.hlasm`, a user can add the macro `MAC` to his source code even if it is in a file called `MAC.hlasm`.

The following example of `proc_grps.json` specifies that files with the extension `.hlasm` are recognized as macros, with the exception of macros in the `C:/external/project/macs` directory, where they must have the extension `.mac`.

```
{
  "pgroups": [
    {
      "name": "GROUP1",
      "libs": [
        "ASMMAC/",
        "C:/SYS.ASMMAC",
        {
          "path": "C:/external/project/macs",
          "macro_extensions": [
            ".mac"
          ]
        }
      ]
    }
  ],
  "macro_extensions": [
    ".hlasm"
  ]
}
```

### Suppression of Diagnostics

For files that use macros extensively but do not have the definitions available, diagnostics reported by HLASM Language Support might not be helpful. For those cases, there is the setting `diagnosticsSuppressLimit`, which can be set either in the editor settings, or in `pgm_conf.json`. For files that do not have processor group configuration in `pgm_conf.json`, all diagnostics are suppressed if they exceed the configured limit.

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
In the `pgm_conf.json` example above, the `source_code` file has a configuration, so all discovered diagnostics are always shown. However, if you open another file and do not assign a processor group to it, its diagnostics are not shown if there are more than 15 of them.

### Preprocessors

Processor groups can be configured so that the HLASM source is processed with a preprocessor. Currently, the following preprocessor options are supported:
- `DB2`
- `CICS` 
- `ENDEVOR`

A preprocessor option can be configured using the `preprocessor` key in a processor group:
```
{
  "pgroups": [
    {
      "name": "GROUP1",
      "libs": [ "ASMMAC/" ],
      "preprocessor": "DB2"
    },
    {
      "name": "GROUP2",
      "libs": [ "ASMMAC/" ],
      "preprocessor": {
        "name": "CICS",
        "options": [
          "NOPROLOG"
        ]
      }
    },
    {
      "name": "GROUP3",
      "libs": [ "ASMMAC/" ],
      "preprocessor": "ENDEVOR"
    },
  ]
}
```

You can also chain the preprocessors in the following way:
```
{
  "pgroups": [
    {
      "name": "GROUP1",
      "libs": [ "ASMMAC/" ],
      "preprocessor": [
        {
          "name": "DB2"
        },
        {
          "name": "CICS",
          "options": [
            "NOPROLOG"
          ]
        }
      ]
    }
  ]
}
```

## Download Dependencies

You can use the HLASM Language Support extension to download dependencies from mainframe data sets specified in `proc_grps.json` to your workspace. To connect to the mainframe, we recommend setting up a `zowe zosmf` or `zowe zftp` profile to store your credentials and connection information.

1. Press **F1** to open the Command Pallet.
2. Run the commend **HLASM: Download dependencies**.
3. Do one of the following:
   - Specify the name of a `zowe zosmf` or `zowe zftp` profile that contains your mainframe credentials and server information, in the format `@profilename`.
   - Enter your server details manually:
      1. Enter the FTP address of your mainframe server in the format `host:port`. The port number is optional.
      2. Enter your mainframe username.
      3. Enter your mainframe password.
      4. Select one of the following security options:
         - **Use TLS, reject unauthorized certificates**
         - **Use TLS, accept unauthorized certificates**
         - **Unsecured connection**   
4. Enter your job header.

All dependencies are downloaded from the specified data sets to your workspace.

### Automatic Dependency Retrieval from Endevor

If you open HLASM source files using Explorer for Endevor, HLASM Language Support retrieves dependencies dynamically from the processor group that is defined in the Endevor element. For more information about Explorer for Endevor, see the [Explorer for Endevor documentation](https://marketplace.visualstudio.com/items?itemName=broadcomMFD.explorer-for-endevor).

## Questions, issues, feature requests, and contributions
- If you have a question about how to accomplish something with the extension, or come across a problem, file an issue on [GitHub](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm)
- Contributions are always welcome! Please see our [GitHub](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm) repository for more information.
- Any and all feedback is appreciated and welcome!
