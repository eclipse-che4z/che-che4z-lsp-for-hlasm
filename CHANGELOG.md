## [0.15.0](https://github.com/eclipse/che-che4z-lsp-for-hlasm/compare/0.14.0...0.15.0) (2021-11-08)


### Features

* Allow wildcards in proc_grps.json library specification ([#172](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/172)) ([9157ef7](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/9157ef78e903c80e484eaa6ec6f6970bd4647093)), closes [#69](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/69)
* Document outline support ([9f65cd4](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/9f65cd4f6a473a349639b49b9836b020ad507788))
* Implement SYSTEM_ID system variable ([#182](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/182)) ([caa6fd0](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/caa6fd04b380a6735a1379ff77f04b25c99b4cc2))
* Support START and MHELP instructions ([#171](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/171)) ([f9f2fb2](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/f9f2fb2852a7feed5e1fdca4a771588b15758762))
* Telemetry reporting ([#187](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/187)) ([70445dd](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/70445ddd3db3d1a8344aa151c7651c5e802459aa))


### Other changes

* Add Support section to client readme ([#192](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/192)) ([c68270f](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/c68270f53cfd9f6bf15289d35d75bb8fa75ddb84))
* replace theia docker images that do not exist anymore ([#197](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/197)) ([ac5cb3d](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/ac5cb3df74f0a80d8cab481469255e251854f3f8))


### Fixes

* &SYSMAC should contain only the macro name (fixes eclipse/che-che4z-lsp-for-hlasm[#168](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/168)) ([4e2fddc](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/4e2fddce850712783d6cfb14091af21692076a53))
* AINSERT operand length validation ([#196](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/196)) ([03d62ad](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/03d62ad65d1ed4167f60f7e41c6319118955dca5))
* Apostrophe parsing in model statements (fixes eclipse/che-che4z-lsp-for-hlasm[#163](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/163)) ([#164](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/164)) ([94843c8](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/94843c8a8facdc621058b00100129331da67e66a))
* Diagnostics lost during JSON serialization ([#170](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/170)) ([f272f7d](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/f272f7d9820488a9ecda52d909679f67507cf2b7))
* DOT operator in string concatenation is optional ([#190](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/190)) ([33d9ecf](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/33d9ecfd14b7a9418418c640d164f7c1f6c73fc0))
* Enhance conditional assembler expression parsing, evaluation and diagnostics ([#181](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/181)) ([40a2019](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/40a2019b59e8fdc240b65da9537145b87e524de1))
* File with extensions for other files should not be set to hlasm ([#173](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/173)) ([fc49775](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/fc497751149b72c2b2a318b618928adabcd18032))
* Fix HLASM Listing highlighting on lines with trimmed whitespace ([#199](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/199)) ([4262e27](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/4262e27861eef1aa7ed87fd8bcac898e6b679bff))
* Incorrect relative immediate operand validation (fixes [#177](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/177)) ([614c86e](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/614c86e844fa7ff47c86b9ceec52f484e0d672aa))
* Infinite loop during lookahead processing when model statement is located in copybook ([#189](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/189)) ([176de31](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/176de31a040fe67e59a8abcf2852a63a3a1af5c1))
* Language server crashes while trying to list inaccessible directory ([60db271](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/60db2715b7a5356a102d80bb22a3add834b00a53))
* Lookahead mode does not work correctly when triggered from AINSERTed code ([#175](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/175)) ([f7143c8](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/f7143c8326f5ddbdea46827e7ccd2a700c841a33))
* Operands of dynamically generated statements may be incorrectly parsed ([#185](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/185)) ([1a9127e](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/1a9127e509cd0cf4137a3bc4a6d92ab742cb616f))
* Preserve mixed-case labels on macro calls ([#165](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/165)) ([d8545fe](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/d8545fee66cf2987d9cdf179529df4d7987d9c6e)), closes [eclipse/che-che4z-lsp-for-hlasm#155](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/155)
* Process END instruction ([#184](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/184)) ([2d5ad75](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/2d5ad7593f78a50483d528568996a4e6af701581))
* Remove ALIAS operand parsing limitation ([#178](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/178)) ([480e602](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/480e602c30fc6cf0cddcd017ee4abe66d75fc043)), closes [#157](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/157)
* Various fixes ([#166](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/166)) ([200b769](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/200b769e5da0082be264971fdc4a916d63426211))
* Write the error name directly to method name of telemetry event ([#200](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/200)) ([6dd6b9f](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/6dd6b9f91cfb10791809c5e4b833a4e1a78e693c))

## [0.14.0](https://github.com/eclipse/che-che4z-lsp-for-hlasm/compare/0.13.0...0.14.0) (2021-08-18)


### Features

* Implement the CCW* instructions ([#154](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/154)) ([f750bcc](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/f750bccc051abea4b1141a71720df109f0a3a670))
* Minimal DB2 preprocessor emulator ([#140](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/140)) ([77275dd](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/77275dd91e8a8ba40ca8a60bd2d47435ccf39ff5))
* Support for complex SQL types ([#146](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/146)) ([3e85b98](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/3e85b98b476eb0b66566a9e729d540fe8fbb8f36))


### Fixes

* AREAD/AINSERT support in macros called from copybooks ([#138](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/138)) ([bdc3718](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/bdc371894730eef021f08f8cadb45f07d622ba6e))
* DB2 LOB locator and file variables are not processed correctly ([20a6fba](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/20a6fba747ca19682806eeadeef1ced851b881ea))
* Dependency files caching ([#129](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/129)) ([2541b7a](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/2541b7a18070e10071b5586827659f6695d54755))
* EXLR flagged as error by plugin ([#121](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/121)) ([e097903](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/e097903698dcafac65c3ccc01faea6a78e9cc85d))
* Inline macros overwriting external definition stored in macro cache ([#148](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/148)) ([93107b3](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/93107b315298ce9b3f5652f28b8ad7a3b8a7dbf3))
* language server crashes while evaluating conditional assembly statements ([#139](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/139)) ([249e85d](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/249e85d2da17caca532d8fadd8517f215a4dbfd4))
* Remove (no longer supported) experimental flags when running WASM server variant on V8 version 9 and newer. ([1cabd76](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/1cabd76999fdffa7a8e700e943813c9f41ac3431))
* Syntax errors reported in bilingual macros ([#152](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/152)) ([a8b1201](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/a8b1201c7cddef5c9d5b5a00b6ae71d3a6e75641)), closes [#144](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/144)
* Various small fixes ([#149](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/149)) ([c1a6896](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/c1a6896ed799efa91f9cc7ff10f5f5d6b7981de3)), closes [eclipse/che-che4z-lsp-for-hlasm#143](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/143) [eclipse/che-che4z-lsp-for-hlasm#142](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/142)
* Various small fixes ([#150](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/150)) ([36fdbda](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/36fdbda1ca93e5a3d4ca173f6253f2b04c6ed53f))


### Other changes

* changelog + readme for preprocessors ([70caf82](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/70caf8292ca64b82f94533de3b83002f42a20168))
* Introduce a  mock usable throughout the parser library tests ([#145](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/145)) ([547948e](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/547948e1a82f701338aa42de57c7f4e18f58c408))
* Parser reports all its diagnostics using single channel ([#141](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/141)) ([656caf3](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/656caf3a21d231713b7d66a8cdbf26091a4a20ac))
* Split complex classes ([#134](https://github.com/eclipse/che-che4z-lsp-for-hlasm/issues/134)) ([78ca7e7](https://github.com/eclipse/che-che4z-lsp-for-hlasm/commit/78ca7e75189df5123379110b2e1bb20b7552143e))

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

