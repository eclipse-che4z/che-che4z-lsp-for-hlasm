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

#### Ordinary Symbols !!

Using EQU you can define named constants and with labels in machine instructions you can define named addresses. DC is not supported yet, neither data attributes are so full experience of ordinary symbols is not provided.

#### External Macro Libraries and Copy Numbers !!

Plugin is able to look for locally stored members (files) when evaluating macro or COPY instruction. Two configuration files are required at the root of opened workspace of VS Code: proc_grps.json and pgm_conf.json. proc_grps.json provides definition of processor groups, which is basically list of folders on computer to tell the plugin where to look for macro and copy files. pgm_conf.json provides mapping between source files (open code files) and processor groups. It tells which list of directories should be used with particular source file. The structure of this configurations was based on Endevor.

The Assembler Language Support extension can look for locally stored members when evaluating a macro or COPY instruction. Ensure that the two configuration files `proc_grps.json` and `pgm_conf.json` are present in the root folder of the opened workspace.

`proc_grps.json` defines the processor groups, which 

`{`
`  "pgroups": [`
`  {`
`    "name": "GROUP1",`
`    "libs": [`
`      "path/to/folder/with/GROUP1/macros",`
`      "second/path/to/folder/with/GROUP2/macros"`
`    ]`
`  },`
`  {`
`    "name": "GROUP2",`
`    "libs": [`
`      "path/to/folder/with/GROUP2/macros",`
`      "second/path/to/folder/with/GROUP2/macros"`
`    ]`
`  }`
`  ]`
`}`
