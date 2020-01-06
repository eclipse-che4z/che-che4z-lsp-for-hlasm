# HLASM Language support

The Language Server Protocol (LSP) for HLASM standardizes the communication between language tooling and your code editor. LSP defines the protocol that is used between an editor or IDE and a language server. It’s functionality can be separated into two major categories, supported language features and supported LSP features. Detailed description of both can be found below.

## Supported HLASM language features
### Conditional assembly instructions
Plugin handles majority of conditional assembly (CA) instructions. It is also able to evaluate them. This means that plugin is fully capable of conditional or unconditional branching, defining global or local variable symbols and recognizing macro definitions. There are implemented only SYSNDX and SYSECT global variable symbols. 

Complete list of supported CA instructions:

AIF
AGO
ACTR
SETA
SETB
SETC
ANOP
LCLA
LCLB
LCLC
GBLA
GBLB
GBLC
MACRO
MEND
MEXIT

### Macro instructions
Expanding macros with passed parameters is fully supported.

### Assembler instructions
Plugin is capable of handling some of the assembler instructions as well.

CSECT
DSECT
LOCTR
COPY
EQU

With CSECT/DSECT plugin only distinguishes whether unique name has been used. Continuation of section with LOCTR is supported.

### Ordinary symbols
Using EQU you can define named constants and with labels in machine instructions you can define named addresses. DC is not supported yet, neither data attributes are so full experience of ordinary symbols is not provided.

### External macro libraries and copy members
Plugin is able to look for locally stored members (files) when evaluating macro or COPY instruction. Two configuration files are required at the root of opened workspace of VS Code: proc_grps.json and pgm_conf.json. proc_grps.json provides definition of processor groups, which is basically list of folders on computer to tell the plugin where to look for macro and copy files. pgm_conf.json provides mapping between source files (open code files) and processor groups. It tells which list of directories should be used with particular source file. The structure of this configurations was based on Endevor.

The proc_grps.json defines two processor groups: GROUP1 and GROUP1 and defines list of directories where to look for macros and COPY files. pgm_conf.json defines, that when working with source_code.hlasm, the plugin should use GROUP1 and when working with second_file.hlasm, GROUP2 will be used. If a path is a relative path, it is relative to the opened workspace (the directory in which the configuration files are in).

So when you want to invoke macro MAC1 from source_code.hlasm, the plugin will search for file with name "MAC1" (the file must have the exact same name as the macro) first in folder "path/to/folder/with/GROUP1/macros" and if that is unsuccesful it will try folder "second/path/to/folder/with/GROUP2/macros". If that is unsuccessful too it will write diagnostic that the macro does not exist.

## Supported LSP features
### Syntax Highlighting
Plugin provides semantic highlighting for statements. It colors labels, instructions, operands, remarks and variables differently. It distinguishes whether instruction can have operands and highlights a statement accordingly. If code is skipped by branching AIF or AGO it will not be colored at all.

### Autocomplete
Autocomplete for instruction field is also enabled. This gives user a list of instructions starting with typed characters. It completes instruction and inserts its operands default operands. Autocomplete is also done for variable and sequence symbols with respect to a scope in which the variables are.

### Go to and find all references
Plugin provides go to definition and find all references for variable symbols, sequence symbols and macro definitions.

### Diagnostics  
Plugin checks for errors in all machine instructions and mentioned CA and assembler instructions and shows them in an IDE.