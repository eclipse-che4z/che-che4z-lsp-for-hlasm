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

Using EQU you can define named constants and with labels in machine instructions you can define named addresses. DC is not supported yet, neither data attributes are so full experience of ordinary symbols is not provided.

#### External Macro Libraries and Copy Numbers

Plugin is able to look for locally stored members (files) when evaluating macro or COPY instruction. Two configuration files are required at the root of opened workspace of VS Code: proc_grps.json and pgm_conf.json. proc_grps.json provides definition of processor groups, which is basically list of folders on computer to tell the plugin where to look for macro and copy files. pgm_conf.json provides mapping between source files (open code files) and processor groups. It tells which list of directories should be used with particular source file. The structure of this configurations was based on Endevor.

The Assembler Language Support extension can look for locally stored members when evaluating a macro or COPY instruction. Ensure that the two configuration files `proc_grps.json` and `pgm_conf.json` are present in the root folder of the opened workspace.

`proc_grps.json` defines the processor groups, a list of folders which contains the location of the macro and copy files.

*Example*:

`{`+
`  "pgroups": [`+
`  {`+
`    "name": "GROUP1",`+
`    "libs": [`+
`      "path/to/folder/with/GROUP1/macros",`+
`      "second/path/to/folder/with/GROUP2/macros"`+
`    ]`+
`  },`+
`  {`+
`    "name": "GROUP2",`+
`    "libs": [`+
`      "path/to/folder/with/GROUP2/macros",`+
`      "second/path/to/folder/with/GROUP2/macros"`+
`    ]`+
`  }`+
`  ]`+
`}`

`pgm_conf.json` provides mapping between source files (open code files) and processor groups. It tells which list of directories should be used with particular source file. The structure of this configurations was based on Endevor.

*Example*:

`{`+
`  "pgms": [`+
`  {`+
`    "program": "source_code.hlasm",`+
`    "pgroup": "GROUP1"`+
`  },`+
`  {`+
`    "program": "second_file.hlasm",`+
`    "pgroup": "GROUP2"`+
`  }`+
`  ]`+
`}`

The proc_grps.json defines two processor groups: GROUP1 and GROUP1 and defines list of directories where to look for macros and COPY files. pgm_conf.json defines, that when working with source_code.hlasm, the plugin should use GROUP1 and when working with second_file.hlasm, GROUP2 will be used. If a path is a relative path, it is relative to the opened workspace (the directory in which the configuration files are in).

So when you want to invoke macro MAC1 from source_code.hlasm, the plugin will search for file with name "MAC1" (the file must have the exact same name as the macro) first in folder "path/to/folder/with/GROUP1/macros" and if that is unsuccesful it will try folder "second/path/to/folder/with/GROUP2/macros". If that is unsuccessful too it will write diagnostic that the macro does not exist.

## Supported LSP Features

### Syntax Highlighting

Plugin provides semantic highlighting for statements. It colors labels, instructions, operands, remarks and variables differently. It distinguishes whether instruction can have operands and highlights a statement accordingly. If code is skipped by branching AIF or AGO it will not be colored at all.

### Autocomplete

Autocomplete for instruction field is also enabled. This gives user a list of instructions starting with typed characters. It completes instruction and inserts its operands default operands. Autocomplete is also done for variable and sequence symbols with respect to a scope in which the variables are.

### Go To and Find All References

Plugin provides go to definition and find all references for variable symbols, sequence symbols and macro definitions.

### Diagnostics

Plugin checks for errors in all machine instructions and mentioned CA and assembler instructions and shows them in an IDE.