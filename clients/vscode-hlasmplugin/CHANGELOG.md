# Changelog

## [0.14.0](https://github.com/eclipse/che-che4z-lsp-for-hlasm/compare/0.13.0...0.14.0) (2021-08-18)

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
- Tolerate '+' in modifiers in data defintions
- Empty TITLE argument must be tolerated
- Statements skipped by conditinal assembly emmiting errors in macros and copy files
- Location counter in machine instruction sometimes evaluating incorrectly
- WASM variant of the language server not working with the V8 JavaScript machine version 9 or later
- Improve language server stability


## [0.13.0](https://github.com/eclipse/che-che4z-lsp-for-hlasm/compare/0.12.0...0.13.0) (2021-06-01)

#### Added
- Support for specifying the SYSPARM option
- AREAD instruction support (AREAD in macros called from COPY member not correctly supported yet)
- Option to tolerate unavailable directories in the macro library configuration (`proc_grps.json`)
- Support for running under node.js, particularly when native binaries cannot be run
- Support for approximately 300 missing machine instructions

#### Changed
- `AlwaysRecognize` option is deprecated in favour of standard VS Code setting `file.associations`. New configuration for macro libraries has been added - option `macro_extensions` specified either for all libraries or for each library separately in `proc_grps.json`.

#### Fixed
- Conversion of empty string into a number in conditional assembly expressions

## [0.12.0](https://github.com/eclipse/che-che4z-lsp-for-hlasm/compare/0.11.1...0.12.0) (2020-12-18)

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
- Instructions that take signed 20 bit displacement are now correctly validated. [#38](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/38)
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
- Configurable wildcards for automatic HLASM language detection [#7](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/7).
- An option to use macros and copy files with custom extensions via wildcards [#4](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/4).

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
- Fixed Windows Firewall blocking the language server [#2](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/2).

## [0.8.1] - 2019-12-16
- Initial release
