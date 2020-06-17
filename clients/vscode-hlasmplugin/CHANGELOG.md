# Changelog

## [0.11.1](https://github.com/michalbali256/che-che4z-lsp-for-hlasm/compare/0.11.0...0.11.1) (2020-06-17)

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
