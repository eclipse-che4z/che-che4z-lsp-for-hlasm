# Changelog

## ****Unreleased****

#### Added
- New document outline implementation
- Fallback to WebAssembly language server automatically

#### Fixed
- Unknown requests were dropped without a proper response

## [1.12.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.11.1...1.12.0) (2024-03-05)

#### Added
- Highlight lines in listings that originate from macros and copybooks
- Navigation actions for listings
- Integration with Explorer for Endevor 1.7.0+
- Folding ranges support

#### Fixed
- Inconsistent completion list with implicitly defined private CSECT
- Incorrect results of string comparison in CA expressions
- Incorrect results of CA conversion functions for multibyte UTF-8 codepoints
- Lookahead not triggered by attribute references in array indices in string concatenations

## [1.11.1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.11.0...1.11.1) (2023-12-04)

## [1.11.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.10.0...1.11.0) (2023-12-01)

#### Added
- Include all available sections in the USING completion list
- Source code preview in the hover text for ordinary symbols
- Show branch direction indicators

#### Fixed
- Exact match might be deprioritized in the completion list
- Diagnostic with out-of-spec range generated
- The language server sometimes crashes while parsing an invalid model operand
- Performance enhacements
- Improved TextMate grammar tokenization

## [1.10.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.9.0...1.10.0) (2023-09-22)

#### Added
- Support floating point special values
- Show active USINGs in the instruction hover text
- Add reachable ordinary symbols into the completion list
- Support Web extension environment

#### Fixed
- Program to processor group mapping precedence
- The language server sometimes crashes while accessing content of virtual files
- "Diagnostics suppressed" informational message is sometimes incorrectly generated
- Auto-select WebAssembly image on platforms without native support
- Querying current directory fails on Windows (WASM)
- Implicit workspaces should not attempt to read configuration files
- Enhanced multiline support in TextMate grammar for listings

## [1.9.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.8.0...1.9.0) (2023-08-03)

#### Added
- Infrastructure for remote configuration
- Implement completion item resolver
- Macro operand completion support
- Support RENT compiler option
- Code action for toggling advisory configuration diagnostics

#### Fixed
- Bridge for Git configuration files may be ignored by the macro tracer
- Source code might not be reparsed after changing dependency name and/or location
- Improve label parsing accuracy and performance
- Attribute references in nominal values are not checked properly
- Incorrect processing of self-referencing data definition statements
- Enhanced multiline support in TextMate grammar for source files
- Better handling of Unicode Supplementary Planes
- Incorrect processor group could be assigned to programs stated in `.bridge.json`
- Almost infinite loop while checking dependency cycles
- CICS status codes substitutions are not always performed

#### Changed
- Performance improvements
- Replaced configuration pop-ups with CodeLens

## [1.8.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.7.0...1.8.0) (2023-05-24)

#### Added
- Implement request cancellation support
- Warning when files with non-UTF-8 encoding are processed
- Support on-demand dependency retrieval via FTP
- Cache contents of remote files

#### Fixed
- Operands no longer classified as remarks when instruction or macro is not recognized
- New or changed files are parsed only when they are opened in the editor
- High CPU usage while going to the symbol definition
- Inconsistent identification of inactive statements
- Closing a dependency without saving does not trigger reparsing
- Source code colorization may be flickering while typing
- Configuration request sent before initialization is done
- VSCode enters an infinite loop of opening and closing files
- "pgm_conf.json not found" prompt shows up even when `.bridge.json` exists
- `.bridge.json` is not reparsed when changed
- Source code not reparsed after missing dependency is provided
- Only refresh libraries that are caching content
- Best-effort support for setting breakpoints in non-filesystem documents

#### Changed
- Programs assigned to "\*NOPROC\*" processor group no longer need its definition in pgm_conf.json

## [1.7.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.6.0...1.7.0) (2023-03-08)

#### Added
- Provide more details about machine instructions in hover texts
- Visually identify preprocessor and inactive statements
- Best-effort navigation for instructions in non-executed macro statements

#### Fixed
- Large macro documentation is not highlighted correctly in hover texts
- Support for concatenated assembler names in DB2 statements

## [1.6.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.5.0...1.6.0) (2023-01-18)

#### Added
- Command for downloading copybooks allows selections of data sets which should be downloaded
- Code actions for an unknown operation code
- Quick fixes for typos in instruction and macro names added to the code actions
- Endevor, CICS and DB2 preprocessor statements highlighting and parsing
- Instruction suggestions are included in the completion list
- Support for the SYSCLOCK system variable
- Implement step out support in the macro tracer

#### Fixed
- LSP requests on virtual files are evaluated in the correct workspace context
- Enhance language server response times
- Missing references and hover text in model statements
- Do not display messages related to Bridge for Git configuration unless it is actively utilized
- Sequence symbol location is incorrect when discovered in the lookahead mode
- Sort variables in the macro tracer
- The language server may occasionally work with obsolete configuration

#### Changed
- Macro label is the preferred go to definition target unless the request is made from the label itself
- Library contents are now shared between processor groups
- File extensions are now ignored when `macro_extensions` option is not provided in proc_grps.json

## [1.5.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.4.0...1.5.0) (2022-11-02)

#### Added
- Add support for z16 instructions
- Enhance documentation for instruction mnemonics
- Support O and T attributes in machine expressions
- Endevor Bridge for Git integration
- Add support for the SYSASM variable
- Add support for SYSM_SEV and SYSM_HSEV system variables
- Line and block comment handling support
- Example workspace enhancements

### Fixed
- Implement rest of the changes introduced by APAR PH46868
- Evaluation of subscripted expressions in CA statements
- Detection of re-declared global variables with inconsistent types
- TITLE instruction name field support
- Produce only a warning when immediate operand overflows in machine instructions
- Relax PROCESS operand validation
- Syntax error while parsing the END language operands
- False positive sequence symbol redefinition diagnostics generated
- Incorrect parsing of multiline operands in macros
- Incorrect implicit length computed for nominal values containing multibyte UTF-8 characters
- Self-defining terms are incorrectly parsed in machine expressions
- Incorrect ORG instruction behavior when operands contain EQUs
- Library diagnostics point to a respective configuration file
- Invalid substring may be generated when conditional assembly string contains multibyte UTF-8 characters
- Utilize alignment information during dependency evaluation
- Incorrect evaluation of the T attribute in EQU statement
- T attribute of a USING label may be incorrect when the label is mentioned in the macro name field
- Missing special case handling when triggering a lookahead mode

### Changed
- DB2 preprocessor produces more accurate output

## [1.4.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.3.0...1.4.0) (2022-08-30)

#### Added
- Command for downloading copybook data sets from the mainframe
- Support for multiple preprocessors
- Preprocessor for expanding Endevor includes
- Code completion for machine instructions and mnemonics utilizes snippets

### Fixed
- Improve performance of file system event processing
- Improve CICS preprocessor accuracy
- False positive diagnostics generated for statements included by the COPY instruction
- Sequence symbol redefinition diagnostic issued even for symbols excluded by CA statements
- Validation of mnemonics with optional operands produced incorrect diagnostics
- Improve parsing performance of CA operands
- Improve performance of dependency tracking and expression evaluation
- Implement changes introduced by APAR PH46868
- Improve performance of generating document outline
- Reduce memory footprint
- The language server may crash when a line is continued by the preprocessor output
- Invalid CA variable definition may crash the language server

## [1.3.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.2.0...1.3.0) (2022-06-30)

#### Added
- Added support for MACHINE assembler option
- Show hexadecimal offsets and lengths in hover texts
- CXD instruction support
- Support for SYSVER system variable
- CICS preprocessor now recognizes DFHVALUE constants
- Enhanced commands for continuation handling and trimming of oversized lines
- Provide the name of a missing variable or ordinary symbol in messages
- SYSIN_DSN and SYSIN_MEMBER support
- Home directory substitution is now supported in `proc_grps.json` and `pgm_conf.json`
- Conditional DB2 preprocessor option
- Visual Studio Code workspace variables `${config:...}` and `${workspaceFolder}` can be used in `pgm_conf.json` and `proc_grps.json`

#### Fixed
- Incorrect attribute values generated when literals are substituted in CA expressions
- AINSERT grammar tests improved
- Incorrect attribute parsing in CA expressions without spaces between individual terms
- String functions are not recognized in concatenations
- Parsing of numeric nominal values must be case insensitive
- The language server may crash when a complex expression is used as a variable symbol index
- References to CA variables in strings are not reported
- Structured macro variables were not forwarded correctly when a dot separator was used in the macro operand
- URIs and paths are now represented by a designated data type
- Return correct variable type for values provided in the macro's name field
- Revise machine instructions
- Incorrect remark parsing in CA statements
- DB2 preprocessor incorrectly processes line continuations from included members
- Partially resolved value used as the final value by EQU statement
- Language server may crash while processing an unexpected operand
- Changes in configuration files were not detected on Linux
- The language server may crash after reloading the configuration

## [1.2.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.1.0...1.2.0) (2022-05-11)

#### Added
- Allow viewing content generated by the AINSERT instruction and preprocessors
- Expanded the list of associated file extensions
- DB2 preprocessor now supports the VERSION option
- Instruction set versioning support (with OPTABLE assembler option)
- Basic GOFF, XOBJECT and SYSOPT_XOBJECT support
- MNOTE support
- Assembler options can be specified in `pgm_conf.json` to override values from `proc_grps.json`

#### Fixed
- Fixed an issue preventing correct N' attribute evaluation of empty subscript arrays
- Contents of subscript arrays are now visible during debugging without the need to expand them
- Improved detection of HLASM files
- Reaching ACTR limit now only generates warnings
- Parsing of negative numbers in machine expressions
- Empty arrays now behave similarly to other subscripted variables in macro tracer
- Structured macro parameters are now correctly forwarded in nested macro calls
- Empty operands ignored by the SETx instructions
- Incorrect operator precedence in conditional assembly expressions
- Incomplete conditional assembly expression evaluation

## [1.1.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.0.0...1.1.0) (2022-03-29)

#### Added
- USING and DROP support
- SYSSTMT support
- Instruction operand checking now utilizes the USING map

#### Fixed
- Improved the behavior of supported subscripted system variables
- Cross-section relative immediate references now only generate warnings
- Incorrect module layout is computed when the ORG instruction is used in sections with multiple location counters
- Immediate variable evaluation now works correctly in statements that use the AINSERT instruction
- Fixed the highlighting of single-character strings that resemble data attributes
- Document navigation does not work correctly in Untitled documents

## [1.0.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.15.1...1.0.0) (2022-01-31)

#### Added
- Literal support
- Location counter length attribute support
- Toleration of EXEC CICS statements and related preprocessing

#### Fixed
- Highlighting now fully works with themes, not just categories dark, light and contrast.
- Incorrect module layout generated when data definition operands have different alignments
- Data definition grammar is too greedy
- Readme update

## [0.15.1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.15.0...0.15.1) (2021-11-11)

#### Fixed
- Compatibility of WASM backend with VS Code 1.62.1

## [0.15.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.14.0...0.15.0) (2021-11-08)

#### Added
- Document outline support
- CNOP instruction implementation (limited)
- START, MHELP instructions
- Wildcard support in library specification
- Support for SYSTEM_ID system variable.
- END instruction

#### Fixed
- Preserve mixed-case labels on macro calls
- Apostrophe parsing in model statements
- Lexer generates the ATTR token only on data attributes that always consume the apostrophe
- Fix instruction formats (STNSM, STOSM, CUUTF, CU21, LLI[LH][LH])
- Evaluation of T'&VAR(num), where VAR is type C-type var symbol array
- &SYSMAC should contain only the macro name
- Diagnostics lost during JSON serialization
- Files with extension should not be set to hlasm in libs folder
- Lookahead mode does not work correctly when triggered from AINSERTed code
- Incorrect relative immediate operand validation
- Remove ALIAS operand parsing limitation
- Attribute expressions ending with dot are not parsed correctly
- Improve evaluation and diagnostics of conditional assembler expressions
- Operands of dynamically generated statements may be incorrectly parsed
- Infinite loop during lookahead processing when model statement is located in copybook
- DOT operator in string concatenation is optional
- AINSERT operand length validation
- HLASM Listing highlighting of lines with trimmed whitespace
- Macro tracer: step over sometimes stops inside a macro or a copy file

## [0.14.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.13.0...0.14.0) (2021-08-18)

#### Added
- Caching of macros and copy files, improving response time of the extension when editing already open file
- Highlighting support for HLASM listings
- Toleration of EXEC SQL statements and support for DB2 SQL types
- CCW, CCW0 and CCW1 instruction support
- Support for EXTRN and WXTRN instructions

#### Fixed
- Handle AREAD in macros called from COPY members
- Format of instructions with relative addressing operands
- Vector instructions flagged when VR16-31 are used
- Improved OPSYN processing
- Language server crashes while evaluating conditional assembly statements
- Allow self-reference in previously undefined array variables
- Tolerate '+' in modifiers in data definitions
- Empty TITLE argument must be tolerated
- Statements skipped by conditional assembly emitting errors in macros and copy files
- Location counter in machine instruction sometimes evaluating incorrectly
- WASM variant of the language server not working with the V8 JavaScript machine version 9 or later
- Improve language server stability


## [0.13.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.12.0...0.13.0) (2021-06-01)

#### Added
- Support for specifying the SYSPARM option
- AREAD instruction support (AREAD in macros called from COPY member not correctly supported yet)
- Option to tolerate unavailable directories in the macro library configuration (`proc_grps.json`)
- Support for running under node.js, particularly when native binaries cannot be run
- Support for approximately 300 missing machine instructions

#### Changed
- `AlwaysRecognize` option is deprecated in favor of standard VS Code setting `file.associations`. New configuration for macro libraries has been added - option `macro_extensions` specified either for all libraries or for each library separately in `proc_grps.json`.

#### Fixed
- Conversion of empty string into a number in conditional assembly expressions

## [0.12.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.11.1...0.12.0) (2020-12-18)

#### Added
- Diagnostic suppression for files that have no processor group configuration.
- Support for light and contrast themes.

#### Fixed
- Parsing issues regarding conditional assembly expressions and attribute lookahead.
- Several issues causing the extension to crash.

## [0.11.1] - 2020-11-09

#### Fixed
- Plugin crashing when used on che-theia 7.21.

## [0.11.0] - 2020-05-07

#### Added
- OPSYN instruction processing.

#### Fixed
- Plugin crashing when writing macro instructions with operands not properly enclosed in parentheses.
- Instructions that take signed 20 bit displacement are now correctly validated. [#38](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/38)
- The VGEF, VGEG, VSCEF, VSCEG instructions were validated with incorrect 20 bit displacement. Unsigned 12 bit displacement is correct.
- Plugin crashing on instruction completion in macros.
- .hlasmplugin folder is now created only if requested via configuration prompt.
- Configuration files are no longer highlighted as HLASM.
- Configuration files contents are no longer reloaded after each change but upon save.
- Files similar to defined programs are no longer being detected.
- Improved automatic HLASM detection accuracy.

## [0.10.0] - 2020-04-02

#### Added
- ORG instruction processing.
- Configurable wildcards for automatic HLASM language detection [#7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/7).
- An option to use macros and copy files with custom extensions via wildcards [#4](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/4).

#### Fixed
- Wrong suggestions for macro names starting with non-alphabetic character.
- Plugin crashing when using % in operands of DS or DC instruction.
- Plugin crashing when referencing an attribute of not yet defined symbol in the instruction field.
- Further improvements in stability.
- VSCode freezing on large files.

## [0.9.1] - 2020-02-06

#### Fixed
- Multiple plugin instances may be run simultaneously.

#### Added
- Dynamic assignment of TCP port for Debugger.

## [0.9.0] - 2020-01-30

#### Added
- Improved performance by average ~50%.

#### Changed
- Updated Readme and license.

#### Fixed
- Fixed "configuration missing" warnings not popping up in specific cases.
- Fixed Windows Firewall blocking the language server [#2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/2).

## [0.8.1] - 2019-12-16
- Initial release
