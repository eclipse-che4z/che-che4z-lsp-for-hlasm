## [0.13.0](https://github.com/eclipse/che-che4z-lsp-for-hlasm/compare/0.12.0...0.13.0) (2021-06-01)


### Features

* AREAD support ([#125](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/125)) ([052c844](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/052c84463cdd2bb390877ae682abca385a4c05ca))
* Provide users ability to use compiler option SYSPARM ([#108](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/108)) ([ccb3a0a](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/ccb3a0a38ef562cbee9ae2348ef278d352ced74a))
* Support for macro file extensions ([#117](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/117)) ([d5b21d2](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/d5b21d2f37b815da3a6bc9dfaa3da3ee5ad87c68))
* Support missing instruction ([#113](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/113)) ([ec547cf](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/ec547cf29a071167e6acffed3eac1900daf8df66))
* Support running under Node.js ([#111](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/111)) ([dc0c47b](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/dc0c47bd756e293c0724c6e4a25b85a632aa3f46))
* UI enhancement for the WASM server variant ([#120](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/120)) ([2d73b0d](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/2d73b0db89b522e8b53fc39b0ed1fc526edd5fe3)), closes [eclipse/che-che4z-lsp-for-hlasm#122](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/122)


### Fixes

* conversion of an empty string to a number ([#119](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/119)) ([b5e6989](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/b5e69896f9daed795e3c0ea62c89dd5eeec3a1bd))
* Fix crash when hovering over non-existing instruction ([3fbb22e](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/3fbb22ee1837fc38ef2709cb5252c447b67e60b2))
* Refactor the way of collecting LSP information ([#110](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/110)) ([d767b6d](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/d767b6d2860fbe90e25d9d5e9e876a1fa8fff4cd))


### Other changes

* Coverity scan ([#123](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/123)) ([daa662e](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/daa662ee8e02f1e5fd41f912c2ab099225faee1b))
* DAP via LSP messages ([#109](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/109)) ([044acad](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/044acad3e5a57610d19492f79b90d759bb7d10d4))
* Implement suggestions from tools ([#118](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/118)) ([b6f231b](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/b6f231b01a8862eb415f6f00a2105fccc5a78934))
* Remove mention of unimplemented PROFILE option ([5f4ce87](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/5f4ce87e653e700331455c04c9307297d633910a))
* Statement refactor ([#93](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/93)) ([0f41701](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/0f41701b3a23405d0e4e63f66700efe2eac22c46))

## [0.12.0](https://github.com/eclipse/che-che4z-lsp-for-hlasm/compare/0.11.1...0.12.0) (2020-12-18)


### Features

* Diagnostics suppression for files with no pgm_conf configuration ([#89](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/89)) ([c287ff1](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/c287ff13e166edf7a191fbf5482a125648b8986a))


### Other changes

* Add example workspace ([#44](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/44)) ([774bb1d](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/774bb1dde75bdaf6456a6bf0b40bceca76760053))
* Add thread sanitizer to CI ([#78](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/78)) ([225b74f](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/225b74f408a29b1288a4f394cd79b23b31dc5295))
* automatic release mechanism ([#53](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/53)) ([0abf1d8](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/0abf1d8a35d41c9095cd57135f789a0e7b4e5036))
* fix obtaining version in theia tests ([23594bb](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/23594bb86207fa70f67ab5bb3e3f8df63fa7722f))
* fix semantic-release not creating pre-release ([9c65e12](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/9c65e12ae598463d994874fa88d5da9ecc511639))
* fix set-env command that is no longer supported ([#95](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/95)) ([c7d061c](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/c7d061cfa0bab6e9c1715c0ec7b2d265f3ff3e71))
* Improve test coverage of Language Server component ([#61](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/61)) ([6c67153](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/6c671533c28e180ddc369b74966af5bbedcc0b1e))
* Integration tests ([#51](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/51)) ([4b3d153](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/4b3d1531014a2601848168df6d52af2eb7e5890c))
* Refactor of attribute lookahead processing ([#84](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/84)) ([ce8e59d](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/ce8e59df8415935be2e007459d62ecd23ee0adad))
* Server test deadlock fix ([#83](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/83)) ([9cc2dfc](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/9cc2dfc33404671df2dea213e146f33ce6ee4e3b))
* TI review of wiki pages and updates to readme files ([#56](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/56)) ([e66ea4a](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/e66ea4a823485335ea58fda14abf024743b95852))
* Update link of join slack badge ([1ec6bfc](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/1ec6bfc7fbf239a8474659c1362e8c3fc642b753))
* wiki inside repository ([#50](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/50)) ([1be2a95](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/1be2a9594e050b49f26c30ea9b3a625cd5942825))


### Fixes

* Conditional assembly expressions ([#65](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/65)) ([99c45ee](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/99c45ee0a45d4b54b4c043e79afb92b73edee935))
* False positives ([#86](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/86)) ([34f3a5e](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/34f3a5ec52e38f3d21471bfb49962e991d348f99))
* Fixed little things in suppression section ([#99](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/99)) ([9374153](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/937415380644e2f26f51ff2fd6b7aa4ab462da41))
* Instruction completion issue ([#64](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/64)) ([8f31888](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/8f3188848a56dd6a1e90aa1a2da107dbeed4e8ab))
* Lexer infinite loop fix ([#85](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/85)) ([027b9f9](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/027b9f99a28ad5a60c9b0f0ed9762a2d643f8610))
* Show ampersands in names of variable symbols and macro parameters in macro tracer ([#79](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/79)) ([2c2338c](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/2c2338c89fb1f7aeb34fdb3315cf102a66ad3ddd))

