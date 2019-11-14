# Assembler Language Support

The Assembler Language Support extension standardizes the communication between language tooling and your code editor. The LSP defines the protocol that is used between an editor or IDE and a language server. The Assembler Language Support extension supports various language features and LSP features.

## Supported Language Features

### Conditional Assembly Instructions

The Assembler Language Support extension can handle and evaluate most conditional assembly (CA) instructions. The extension supports conditional and unconditional branching, defining global and local variables and recognizing macro definitions. The only global variable symbols implemented are SYSNDX and SYSECT.

The following CA instructions are supported:

* ACTR
* AGO
* AIF
* ANOP
* GBLA
* GBLB
* GBLC
* LCLA
* LCLB
* LCLC
* MACRO
* MEND
* MEXIT
* SETA
* SETB
* SETC

### Macro Instructions

The Assembler Language Support extension fully supports the expansion of macros with passed parameters.

### Assembler Instructions

The Assembler Language Support extension can handle the following assembler instructions:

* COPY
* CSECT
* DSECT
* EQU
* LOCTR

#### CSECT and DSECT

When evaluating a CSECT or DSECT instruction the plugin only distinguishes whether a unique name is used.

#### Ordinary Symbols

You can define named constants using EQU and named addresses using labels in machine instructions.

DC and data attributes are not yet supported.

#### External Macro Libraries and Copy Numbers

The Assembler Language Support extension looks for locally stored members when a macro or COPY instruction is evaluated. Ensure that the two configuration files `proc_grps.json` and `pgm_conf.json` are present in the root folder of the current workspace.

**Example (`proc_grps.json`)**:

The following example defines two processor groups, GROUP1 and GROUP2, and a list of directories to search for macros and COPY files.


    {
      "pgroups": [
      {
        "name": "GROUP1",
        "libs": [
          "path/to/folder/with/GROUP1/macros",
          "second/path/to/folder/with/GROUP2/macros"
        ]
      },
      {
        "name": "GROUP2",
        "libs": [
          "path/to/folder/with/GROUP2/macros",
          "second/path/to/folder/with/GROUP2/macros"
        ]
      }
      ]
    }

`pgm_conf.json` provides mapping between source files (open code files) and processor groups. It specifies which list of directories is used with which source file. If a relative path source file path is specified, it is relative to the current workspace (the directory in which the configuration files are in).

**Example (`pgm_conf.json`)**:

The following example specifies that GROUP1 is used when working with source_code.hlasm and GROUP2 is used when working with second_file.hlasm. 

    {
      "pgms": [
      {
        "program": "source_code.hlasm",
        "pgroup": "GROUP1"
      },
      {
        "program": "second_file.hlasm",
        "pgroup": "GROUP2"
      }
      ]
    }
    
If you have the two configuration files configured as above and invoke the MAC1 macro from `source_code.hlasm`, the path `path/to/folder/with/GROUP1/macros` is searched for a file with the exact name "MAC1". If that search is unsuccessful the folder `second/path/to/folder/with/GROUP2/macros` is searched. If that search is unsuccessful an error displays that the macro does not exist.

## Supported LSP Features

### Syntax Highlighting

The Assembler Language Support extension provides syntax highlighting for statements with different colors for labels, instructions, operands, remarks and variables. Statements containing instructions that can have operands are highlighted. Code that is skipped by branching AIF or AGO is not colored.

### Autocomplete

Autocomplete is enabled for the instruction field. While typing, a list of instructions starting with the typed characters displays. Selecting an instruction from the list completes it and inserts the default operands. Variables and sequence symbols are also filled with a value from their scope.

### Go To and Find All References

The Assembler Language Support extension provides go to definition and find all references for variable symbols, sequence symbols and macro definitions.

### Diagnostics

The Assembler Language Support extension checks for errors in all machine instructions and supported CA and assembler instructions and displays them in the IDE.
