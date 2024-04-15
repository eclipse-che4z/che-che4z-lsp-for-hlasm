The role of the analyzer is to provide a facade over objects and methods to create a simple interface for lexical and semantic processing (analysis) of a single HLASM source file. The output of the analysis is the basic input of the LSP server.

After the analyzer is constructed, it analyzes the provided source file. As a result, it updates HLASM context tables and provides a list of diagnostics linked to the file, highlighting, a list of symbol definitions, etc.

Overview
--------

The analyzer is composed of several sub-components, all required to properly process the file.

- The **LSP data collector** collects and retrieves all LSP information created while processing the file.
- **HLASM context tables** hold information about the context of the processed HLASM source code.
- **Lexer–Parser sub-components** simplify the processing interface and ease the use of this component. They are needed to create a source file parser.
- The **processing manager** executes the main loop where the file is processed.

The LSP data collector is required by the Lexer-Parser sub-components. They are composed into the parser object required by the processing manager. HLASM context tables are used by the manager and the sub-components as well.

The components together contribute to the proper functionality of the method `analyze`. It processes a provided file and fills the LSP data collector, from which LSP information can be further retrieved.

### Construction

In order to parse a HLASM file, the analyzer class is constructed with the following parameters:

-   *Name and content of the file.*

-   *Parse library provider* – an object responsible for resolving source file dependencies. The dependencies are only discovered during the analysis, so it is not possible to provide the files beforehand.

-   *Processing tracer* (see [[macro tracer]]).

When this constructor is used, the analyzer creates HLASM context tables and processes the provided source as an open-code. The analyzer has *owner semantics*; it is the owner of the context tables.

The analyzer provides *reference semantics* as well (holding just a reference of the context tables). The provided source is not treated as an open-code, rather as an external file dependency. The constructor of an analyzer with reference semantics adds the following two parameters to the previous one:

-   *HLASM context tables reference* — belongs to the owning open-code analyzer.

-   *Library data* — states how the dependency file should be treated (see [[Processing manager]]).

This constructor is called within the open-code analyzer by its sub-components when they use the *Parse library provider*.

The components of the analyzer are further described in the following pages:
1. [[LSP data collector]]
2. [[Processing manager]]
3. [[Instruction format validation]]
4. [[Lexer]]
5. [[Parser]]
6. [[HLASM context tables]]
