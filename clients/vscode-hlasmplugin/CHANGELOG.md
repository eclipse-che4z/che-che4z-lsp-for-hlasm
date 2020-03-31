# Changelog

All notable changes to the HLASM Language Support extension are documented in this file.

## [0.10.0] - 2020-03-16

#### Added
- ORG instruction processing.
- Configurable wildcards for automatic HLASM language detection [#7](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/7).
- An option to use macros and copy files with custom extensions via wildcards [#4](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/4).

#### Fixed
- Wrong suggestions for macro names starting with non-alphabetic character.

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
