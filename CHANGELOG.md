## [1.17.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.16.0...1.17.0) (2025-03-31)

### Features

* Navigation through listings using label names and CSECT offsets ([a96a76b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a96a76b04a727b30bcaf7823976983af0d40b8a5))
* Progress indicator ([5d4a022](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5d4a022109bec1802ea8d1ab705c06b52d6b0904))
* Support analyzing files opened directly by Zowe Explorer ([39d3529](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/39d3529d8327a0753162d18ca4fc395ff0fd7938))

### Fixes

* Incorrect handling of scopes in copybooks ([896e328](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/896e328aa2236b6aeeeb1b097d3fc4f9dba873ef))
* Incorrect stack reporting ([0104633](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0104633d6aa6a096ccddd6297e6f6e8f328db931))
* Missing character check in CA string parser ([7ee10c2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/7ee10c2a3dfc0f309f8594dfea5f450dcc11c57f))
* Symbols in listings with ASA characters are incorrectly isolated ([2089b07](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2089b072427ced60635685dda64bccd79433e09f))
* Workaround for MSVC 17.13 ([562ddf6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/562ddf69c110707f6af9dd5a2b1aa2cd1b79973f))

### Other changes

* Merge line details structures ([873dfc1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/873dfc1339c1d888e67138abe0a6ce46f86ece59))
* Simplify constant machine expression ([5ca5713](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5ca5713021caba3b294bc9ba7623626f838bf03a))
* Update Sonar scan ([729569f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/729569f2de0afb74ed4d995428e326d0e44c6cac))
* Upgrade CMake and CI ([469312a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/469312a93b98148b352ca421d7b7df4339334cb7))
* Use string_view in notify method ([7c3dddc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/7c3dddc3d0273cc759c5726adad1b5c9a8c3b3ae))
* Use UTF8 in the parser ([8f3e5f3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/8f3e5f3971d15daa2cf1b9d6ac9cc0941b4fa8cd))
## [1.16.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.15.1...1.16.0) (2025-01-29)


### Features

* New parser implementation ([b91f166](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b91f166eb17b9fdccc7dcbc1fc64492470d03961))


### Other changes

* Address const correctness of diagnostic collectors ([058a4c0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/058a4c07ac19c6822c3db3a3198e28a52881ad7c))
* Do not scan generated source code ([d349fe3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d349fe32a8fd2abc3213c88eb46bfbb58da2b6e2))
* MacOS 12 deprecated ([190db1f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/190db1fda3a0a3bdef918dd4eab8e428dec1635b))
* Move header ([b8cf91c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b8cf91c08b68878ccb72522c7b112cd55c94b1e2))


### Fixes

* Infinite loop on invalid preprocessor operands ([815ef61](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/815ef61e0bada782daa778fad4fd7df77a128087))
* Infinite loop when an incomplete data definition is found in the lookahead mode ([da99fd7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/da99fd7d9e4330b849493eab4cf8ad07699f8a35))
* Infinite loop when an invalid EQU operand is found in the lookahead mode ([b40887d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b40887dfaca39fe04c8e5da1c9e16d71b09abdbc))
* Origin of macro statements is tracked incorrectly ([aa59bd6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/aa59bd636856e3efa5ee1b82ef23f73e42c9e199))
* Origin of macro statements is tracked incorrectly for nested macros ([b76ffa1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b76ffa1f3f6896e4eea70a985411458dda7b9b5c))
* The language server crashes due to incorrect handling of dependency cycles ([59be123](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/59be123b3c3dbc00da8805846f253e9f162defce))
* The language server crashes when a redefined data constant has a dependency ([4254c4f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4254c4f3f58a69da260c042daf8bf8f23b825541))
* The language server crashes while processing data constant with cyclic dependency ([c112c7a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c112c7a6312bd210e7e1d0b185ffb9254570bdc0))

### [1.15.1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.15.0...1.15.1) (2024-11-22)


### Other changes

* Add extra tests for deferred statements ([9c908ac](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9c908acf63e7427aaf9ed27043c276192512045e))
* Line continuation handling in lexer ([3ac3088](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3ac3088e96d00960798c298b4da05b6b281fb833))
* Line limits collection ([4c6b11e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4c6b11eef0c4412df45c70efebeac81908d55708))
* Minor lexer cleanup ([552035b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/552035bf714ae78bb9b186aacf98bd308f85276d))
* Remove input source ([cd0c460](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/cd0c460b262be4f2491a2cdd560eb81f32c56ddf))
* Streamline macro operand grammar ([f82a055](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f82a05580728052b92abf6b1b88c57701cbbb314))


### Fixes

* Consume does not work correctly while on a hidden token ([17cf881](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/17cf881e36c931ba866c9e8ba754c5f5b18e3c18))
* Diagnostic issued for a valid flag parameter on the  statement ([820bf68](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/820bf684959808e8dc08988c5d68722cc00a6dfb))
* Incorrect parsing of attributes in macro operands ([ef046d5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ef046d5e668fb6aefaf40651b0910a60695023c3))
* Language server does not recognize symbolic links as available macros (WASM) ([e28d3dc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e28d3dc3748ace3fcea1b6af10bb396aa5f361fb))
* Operand parsing errors reported for operandless instructions used in macros ([ad7cbd3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ad7cbd3f3d8783ae85db313b8d462adff58d0ab8))
* Single comma operand should be treated as no operands during validation ([532e634](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/532e634a515b5a7b34de3a39c111071c71b84a58))
* Syntax error diagnostics issued due to inconsistent lexer initialization ([eabeb1c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/eabeb1c62db7866a9ccba16cc80b33cfc33c7424))
* The language server crashes while processing an invalid prototype of a nested macro ([9e0cab3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9e0cab350c9875be33a25d9298e511c277895281))

## [1.15.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.14.0...1.15.0) (2024-09-20)


### Features

* Support CATTR, XATTR and R-type constants ([85f51dc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/85f51dc2efcf26de2062ccc515b4ddeeb42493cb))
* Support comments in configuration files ([6bb7283](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6bb7283f7c7391746b3bfc7665727530d24e1d88))


### Other changes

* Clean up WASM glue code ([bc04cd9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/bc04cd9ff246dc856ebad3aedf1ff53cf127a13c))
* More resource_location cleanup ([23b5909](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/23b5909b0736a1d3d0e25b22dc99f144aa90c61e))
* Remove highlighting symbol production from lexer ([92d1915](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/92d19153f3ee6c4a2938cd4022d069d2617a9a4f))
* Remove redundant arguments ([bad80a8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/bad80a84fe1b6bc24cb4ea9ca389ae2cdb9eefa8))
* Remove resource_location deduplication ([19e29fa](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/19e29faf7cf2613bb70cdc8a3b2676d4aaa5b1a1))
* Remove unused field ([24e38f0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/24e38f0203a517289d5b71c8e0432aa4750367f5))
* Start extension before adding tests ([84d24f7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/84d24f714be5e6f45c04df9ec500183c7fd7111b))
* Support Linux on SystemZ ([4a19fe4](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4a19fe4b35b37696795b6eb408c2445ef5948bd4))


### Fixes

* Avoid allocation in stack frame tree ([ff3ad99](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ff3ad999a6834d904a13a142992d75958540441c))
* Deferred operands ([7994a21](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/7994a212d654fc99f51dcba1358db997cbefca7a))
* Download dependencies command supports reading processor groups from settings ([afc14a7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/afc14a7198004e06f096d4f96f91e14feac1bdbe))
* Evaluate relative paths specified in settings with respect to root folder ([962da20](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/962da205188892c40823550122570d20b385f607))
* Identify error states indicating a data set does not exist ([70dc8ca](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/70dc8ca38425ce78d719ac3e9e0f57de765bd4a0))
* Macro operand parsing ([dbbe8ac](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/dbbe8ac92dab9a9049948f348993d57572423504))
* PRINT instruction should tolerate null operands ([642ec23](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/642ec23ac30724b58033433bf062fa1722e67735))
* Retrieval of dependencies is suspended by an arbitrary error ([1ac9985](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1ac9985786a94cde4fac805507062c85f3c14e03))
* Simplify macro operand preprocessing ([88be6eb](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/88be6ebffe72d7cebe60762df3b9c75316858d81))
* The language server crashes due to an invalid CATTR or XATTR instruction operand ([9bcb14f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9bcb14fbc860aebfc718f4c70d2ba4ceeec4d81e))
* Unify error handling ([911d6a3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/911d6a33eda6672654d36c0115db305e7af60d36))

## [1.14.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.13.0...1.14.0) (2024-07-16)


### Features

* Support configuration in user or workspace settings ([610c04c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/610c04c6902404d4f98bf0ffcedb2977b22a28f1))
* Support LSP actions across workspace folders ([385b6b8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/385b6b8a926612484e7552db5c393c8de18c9df5))
* Validate even-odd register requirements of machine instructions ([6575b89](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6575b892e4d4ff132cdfbb028504d59bd8f2e0a8))


### Other changes

* Analyzer interface ([2feba81](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2feba81d9169ac6b5eabb2e0f38a37210ae27bf8))
* Disable issue and PR commenting ([3e31e4c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3e31e4c3c5a760a66a2f669f4db00a858f7370c5))
* Header verification ([2a52672](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2a526729f05586de8c226dcad2399f7e6e664f8d))
* Implement workaround for MSVC 17.10 (cl 19.40) ([b9c590a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b9c590a3083cec6579e97956736ce9d6a840bc82))
* Operand checking ([5879157](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/587915746686ae14530a018d347e9553d09d6b8a))
* Prune statement class hierarchy ([52fa34a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/52fa34a83139e80f61973ef128a496e8757c4238))
* Reduce indirections in addresses ([0d5aee8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0d5aee8c85bf880e2e8f70a24d5cdda2c4a17a3a))
* Remove C++ 20 workarounds ([08b4313](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/08b43132711ca96f2cb2b9eeaefdc192c8e82a43))
* Remove deprecated runners ([69fdeb5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/69fdeb561b992752218474054f94ffa5487ea10c))
* Remove proxies ([81c1bb1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/81c1bb1068170c15b6fefb80fc9f103caffb281d))
* Remove redundant copies ([2d8b98d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2d8b98d418ba1f706f2823e675cfef139d2cda47))
* Reorganize location counters ([5160360](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/51603609e7654ef71368cf0bd0f6f31d1110b8a8))
* Upgrade compilers ([a888343](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a888343c0fb74dbe101df0ccca3139606c062d24))
* Use ranges ([ec6b856](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ec6b8568ba3bfd4fccf13d0927aa4804cdbd81d5))
* Use std::format ([bdde5a9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/bdde5a9bac7a13443dd3d990fa3f0e5a665bf815))
* Use unordered_map in macro cache ([ad0c447](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ad0c447ccf270dbd0243bee51e5b26412d38b44a))


### Fixes

* Accept additional URI schemes based on currently opened workspaces ([ddf51a0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ddf51a06bc0308841abafdfa788ecfc5967498db))
* Incorrect subtraction operator ([0a9c619](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0a9c61950fd4c170d8a7d2b6391329bc43e1a018))
* Postponed statement handling ([5427ca0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5427ca0a9d8e727a4bdd4b2402d7c48437a3b800))
* Report external configuration activity ([1ffae9b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1ffae9bb19cea6f9c8d56e3854fe9e39c4929fde))
* WASM packaging problem ([a4b4674](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a4b4674e768ae1932fd984b44f442f8d086c502a))
* Web Extension fails to load in VSCode 1.92.0 ([1dc6237](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1dc62374eccff2781e3b6f7ebc2520dbda361519))

## [1.13.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.12.0...1.13.0) (2024-04-24)


### Features

* Emit MNOTE and PUNCH arguments to the debug console ([67cfec3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/67cfec34d76d70d40e73bb8b33db01132154197d))
* Fallback to WebAssembly language server automatically ([5230327](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/523032773f14675f7ca40b98500a1c88e0f88e70))
* Function breakpoint support in the Macro tracer ([c8f45a6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c8f45a6d5d8c1aa8dc8e96183c3170b69f9e7815))
* Make MNOTE and PUNCH outputs available from VSCode ([3665e6a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3665e6a1e69dc61af9a2e241c05b7b863d415a31))
* New document outline implementation ([277e044](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/277e0447405610ba3aeeac665a915579aa1c69f0))
* Reaching ACTR limit pauses Macro tracer session ([e21ea32](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e21ea32346e8b0954b8b121a55bfa84171769f12))
* Watch support in the Macro tracer ([e265836](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e26583615fcf77c1cc1caec7775e6b658b25bae9))


### Other changes

* **github:** Reduce ASLR entropy ([d932a45](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d932a4544affa48fa62959507162fa9cfd4149c8))
* Hide diagnostic collection details ([7c5d260](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/7c5d26089c4ef14cf4c7c2d87160bea314b97f20))
* Move constants ([dc08a9b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/dc08a9bd4bea9ddc5d59715a31c4837b73cd977f))
* Publish individual language server binaries ([438e7c5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/438e7c52249e11f457f244108edddab2e998ba53))
* Reduce large structure moves ([6749e73](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6749e73e10f09096a721b36c89df7288eafbe947))
* Remove unnecessary copies ([7d929f5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/7d929f5e57357c9e13eeb280b4e8ab9fd565c9a8))
* Remove unneeded catch-all blocks ([ea812ce](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ea812ce45859f81382afee395a927d9678bc712b))
* Run UI tests with sanitizers ([5f977e6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5f977e69b8ab4f789e48ef0fde982bb93318889a))
* Simplify debugger variable representation ([baabb42](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/baabb429c6d1042fc91408f14c50790fa52a9cb1))
* Split instruction completion table ([8538a72](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/8538a72a43d7d02dd44ad4b900aaef08c1e09a90))
* Support MacOS on ARM64 ([8b10e58](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/8b10e582b38713cb1aefde652439286bcd6b1b60))
* Support Windows and Linux on ARM ([5aaf7b9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5aaf7b9dc57b3f29cc9e5c32bd9f2a0fadbea425))
* Tolerate BUILD_SHARED_LIBS=On ([b8119f4](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b8119f44c034c16751b08a40d3abdf3f3e543f08))
* Upgrade diagnostics matching ([afceacc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/afceacc8bcca7662df38c94bd33185e0b43d433f))
* Use more string_views ([965c274](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/965c274bc322974930a844d3aae1f480f746949c))


### Fixes

* Debugger does not correctly diagnose invalid expressions ([a024556](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a0245568d108a40658b015142dd44e3f51dc2368))
* Expand Neovim configuration scripts ([d5b341e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d5b341e90fca52f8504f404debcc94c0f2f752be))
* Hover text for complex relocation symbols contains incorrect multiplicative factors ([82ebcd6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/82ebcd65f7e659c20711662720e9be5bc0da1853))
* Produce more specific diagnostic on grammar predicate failure ([28ed301](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/28ed301a72b37acbcabc6f82fb2958e986bc9d6f))
* Unknown requests were dropped without a proper response ([9366130](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9366130c070f9bd8be9026f3955b2a37baecf5f2))
* Use Title Case for Command Names ([6fd038c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6fd038cb8a0c410558838a310d0340df84490130))
* Using LSP dynamic registration of file change notifications ([dc43d9a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/dc43d9a39806035bce69b2c2ea4b8e1860b0ec1c))

## [1.12.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.11.1...1.12.0) (2024-03-05)


### Features

* Folding ranges support ([32b2500](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/32b2500d78b0652be5afc67227c212c77153be4e))
* Highlight lines in listings that originate from macros and copybooks ([febdcb8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/febdcb8a8d559d998a272228e4c69e201d787846))
* Integration with Explorer for Endevor 1.7.0+ ([43c91db](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/43c91db05898254c9f381f9365bdf095190b7086))
* Navigation actions for listings ([21a63a5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/21a63a5be430c3d00f26a248f09a46fd871cc575))


### Fixes

* Adjust CA expression grammar ([7a9c523](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/7a9c523dcdaf2dde362f6ed744ad8b144e3ea47e))
* Avoid duplicate collection of CA instructions in macros ([c628cce](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c628cce16107d7a3d6a978231ad093d899e0f131))
* Consider scope for SYSM_SEV value in debugger ([d5e0bb2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d5e0bb27bfe057db80407146054eb57f3e4cd0e5))
* Inconsistent completion list with implicitly defined private CSECT ([ae23b97](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ae23b97e19db5961dd7fd3d295431ba625c9acba))
* Incorrect results of CA conversion functions for multibyte UTF-8 codepoints ([ad91834](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ad91834260b0e97361410c6808cffd9492bc60d9))
* Incorrect results of string comparison in CA expressions ([9c1ec71](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9c1ec7134b71cf43aef34c4e6cd88dded9d9b929))
* Lookahead not triggered by attribute references in array indices in string concatenations ([40f01c4](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/40f01c4d91ec3c884279436e150280a1f23d8bfe))
* More efficient line tracking ([a50a041](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a50a04127e319c86ff6e99fa48f36610d70b2c48))
* Rework system variables ([9e6f036](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9e6f036cf0941d93c2635837d30c4f2e3ea170a1))
* Server startup failure not reported ([9b9c78b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9b9c78b1c9c3ec52c6888cfabff31fc5c45950ec))
* Trivial symbol_occurrence structure ([8c85623](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/8c85623d980505feaaad923faac04c79f37d0ade))
* Use raw URI path in external files ([c80ae7d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c80ae7d613508bd8819aef57aecfaf7c220f2800))


### Other changes

* CA statement processing ([af695b5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/af695b57d76577e651b11e498b3f695b9ec0e226))
* Remove broken node workaround ([2fb8c41](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2fb8c41e97d0110f7e9564013583dca444975640))
* Remove unused owning shared pointers ([3303f23](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3303f23c9ecbfeb7305a8670372f0a532cfe989e))
* Reorganize variable ownership ([1f2f315](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1f2f315c1db0b885959339415e204b6b34f43026))
* Run more tests in the browser environment ([e32d812](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e32d812e82f65afb063cc5369fe4a1386488ef8a))
* SET_t structure cleanup ([652f507](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/652f507874857094d2d8d5e5acc27da13f6edc2a))
* Upgrade deprecated dependencies ([8de6e4d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/8de6e4dc17b382917c3f3d9ac9465128ddf92b0f))

### [1.11.1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.11.0...1.11.1) (2023-12-04)


### Other changes

* Incorrect parameter passed to vsce package ([14dbd5c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/14dbd5c2a6d496a201f1a147bfc280048e3e8215))


### Fixes

* Release without pre-release flag ([024afdc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/024afdc258a7cad691c316336cc908ca1e77dfa4))

## [1.11.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.10.0...1.11.0) (2023-12-01)


### Features

* Include all available sections in the USING completion list ([5da50a9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5da50a9fb521d135f470681f95bff203a33ad390))
* Show branch direction indicators ([db848fe](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/db848fecf9be8d90ea63f77236e28bacd54571f8))
* Source code preview in the hover text for ordinary symbols ([a0536ce](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a0536ceffa726a08aa78679c15dd141383ed7401))


### Fixes

* Benchmark URI issues ([4f77074](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4f770748ebbd2d8433ce28afd7a6e23e704975ea))
* Diagnostic with out-of-spec range generated ([8298546](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/829854600ab8858f852bd81d13e2f90616992e8a))
* Exact match may be deprioritized in the completion list ([11f437a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/11f437a9fabaf2af21a2cad545779c235f5d1614))
* Improve remark parsing ([ffd6d7a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ffd6d7a58f69a4ea38ee4e19d814fb29d578cc41))
* Improved TextMate grammar tokenization ([1223bc5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1223bc56b59ebe6adacfa8df16c7ddfc5fe64c78))
* Provide specialized parser recovery for lookahead mode ([cace56e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/cace56ed2f190ec7699d6282d9013b79d1728862))
* Remarks grouping in TextMate ([49eb035](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/49eb0353e6acf60d1a33dd91c429176dbc100356))
* Simplify single-line grammar parsing ([625a8b0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/625a8b02617a60f9db8f65ce8c18e3206485fa32))
* Streamline ca expression evaluation ([484d6b1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/484d6b12fce648715771e1a63fdc1a3e89d7ecac))
* The language server sometimes crashes while parsing an invalid model operand ([d76e2db](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d76e2db0a9d0d7cb5dcfc05580e30dfe425b954a))
* **windows:** Poor toupper performance ([e6dfff1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e6dfff19ce59ec9ed1055d0f3b6f88d79b2a087b))


### Other changes

* Add completion list consistency test ([45a8f02](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/45a8f02e72ef98e6af4839f6b8fb3fbee22432e9))
* Broken vscode test package ([a5e2dfe](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a5e2dfe34bf8f5d1553eb7c8bfa04097668eb23f))
* Bump clang version ([a34a339](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a34a3395ea6ce9ff217a73f5d0e71aba8fee9c24))
* Mark non-production packages as pre-releases ([9aa8b08](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9aa8b08e24dfeb649f3bf106f0465d13c0bbafe7))
* Reduce data definition operand allocations ([8712149](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/871214974002a2d72cb8655aa4d35f6b59641875))
* Remove sequence symbol map from the code stack ([dd5fca9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/dd5fca9082eef58fce47403b8adeb6261e360316))
* Reuse token_info buffers ([44cdcad](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/44cdcadb09ce52b7e4a37d030d433f44d07af798))
* Rework token stream ([6079dd4](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6079dd43fd54263eefcde727a12012cf9edc1735))
* Streamline grammar ([9e0dddc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9e0dddcdb8f08ed36d79445cd7471f4d49c0b8af))
* Strong type for statement index ([9df8c37](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9df8c37d4a1b347ae42e87efb65c77b0d7f2cbb6))
* Unify machine instruction and mnemonic lookup ([32e986c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/32e986c11b6bb4067dbe9d6b5a379a08338448ae))
* Upgrade image versions ([98c351d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/98c351d36fe800c9d23223e5b90cfcc2545a9bad))
* Variable indexing ([4d3aa76](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4d3aa76030086d9a0c8c0241cc85b5cbbb142b0d))
* Workaround for broken node 18.18.0 ([cfdb538](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/cfdb5383c4937f0bce51b2c9bea79bc1f9a1c4d4))

## [1.10.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.9.0...1.10.0) (2023-09-22)


### Features

* Add reachable ordinary symbols into the completion list ([41ba219](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/41ba219bb59b6eebb942c532ba4f2db5e7803055))
* Show active USINGs in the instruction hover text ([bbe0c8b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/bbe0c8b63111757406e987af7a3c98db4102081a))
* Support floating point special values ([de15a8c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/de15a8cf8be8232c0725e8e2e47fda930a9249d3))
* Support Web extension environment ([30c2fb2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/30c2fb2c49972a624b9f59a51cfc00505421af13))


### Other changes

* Cheaper address copies ([0d4d8ec](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0d4d8ec082abf2cb2353b9c9928f0c3334baba50))
* Cleanup headers ([c9af94a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c9af94a34efb0480731388991b8d6f2083d65918))
* Enhance LSP implementation compatibility ([866fc6d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/866fc6df2545dcf17d1849b14af1459209ebe232))
* External files upgrade ([a7b2fd3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a7b2fd3f0eb772036986ddcd37b688c48805bf69))
* Remove Theia compatibility layer ([c3a888e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c3a888e2dfe96d90b8b0ea83c17afc055fe1b713))
* Resolve deprecated sonar.login and java version ([fb1b54f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/fb1b54f65a4dec3ce651f249b3ed9857363165e2))
* Simpler opcodes structures ([5196287](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5196287cfb56963067cad1e41ad6fa0392f302ab))
* Simpler structure for lookahead set ([9d50f09](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9d50f092425d5708782faa03b126c2831f381609))
* Sonar scanner update ([1c35006](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1c35006750dd85e63c8cf91964f026d4352bc19e))


### Fixes

* "Diagnostics suppressed" informational message may be incorrectly generated ([1c94cc6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1c94cc6bda75677fc3050cc280f1a7543119bd7a))
* Auto-select WebAssembly image on platforms without native support (fixes eclipse-che4z/che-che4z-lsp-for-hlasm[#283](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/283)) ([bcf91c9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/bcf91c9e90318b760224cbe0ab0678abf89049c2))
* **ci:** Minify client code ([807ee77](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/807ee77e93c8d71557714afcfc8bf4e197fd5563))
* Enhanced multiline support in TextMate grammar for listings ([ebed0f3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ebed0f3795ee8c144587f86762bc218e02580312))
* Implicit workspaces should not attempt reading configuration files ([239bf21](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/239bf2166d18a476f7386a1ca36ddddb5ce2fbd3))
* Incorrect items in the completion list ([fed9950](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/fed995048d81cd8d1c851b8e18ea0ce647d378d8))
* Make address operations less expensive ([5bcdc67](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5bcdc6757fb2aac42d8d19672350000e1dd33498))
* Program to processor group mapping precedence ([ad1c27c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ad1c27ccf3d6f029062a6efba715ae301d5bed27))
* Querying current directory fails on Windows (WASM) ([75e077b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/75e077b3c7bb2e8404a09c721d9dff9604f93651))
* Rework symbol dependency structures ([5ce2c92](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5ce2c929b583f88764c26db2d2b1ba9cf12047ee))
* The language server may crash while accessing content of virtual files ([a7fc721](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a7fc721cdd3e2e61bccbafc88e0991eb02b7ae60))

## [1.9.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.8.0...1.9.0) (2023-08-03)


### Features

* Code action for toggling advisory configuration diagnostics ([e72f51f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e72f51fa2b904038059fac437891feff4540bc30))
* Implement completion item resolver ([0301a2f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0301a2fdf93001a05fa778e5682e8a9a925a791f))
* Infrastructure for remote configuration ([818881d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/818881d9cb9f875864a50734fc159b05aa4f9db1))
* Macro operand completion support ([08b297c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/08b297c7f21437cfc3ed3558c4600c2d2f4fc342))
* Support RENT compiler option ([e2a5d85](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e2a5d8590561fd27cec25627b06a19349fc93c0d))


### Other changes

* Cleanup std::bind ([fd86c3a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/fd86c3a1e45506ee355aabddc8c6cb540ac4157a))
* Code actions + configuration nodes ([0a6b5e9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0a6b5e9ddd5b195e52527c557649858d19e43191))
* Collect undefined attribute symbols ([b372630](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b3726302970de430a21d5e49ee2fc754bc4f5878))
* Ensure consistent behavior of multiline commands ([2466c06](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2466c06a6dbd6c5208946fc2f1e4cadefe7bd38f))
* Remove obsolete tokens ([e1c4378](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e1c43783954956a66570c16a77565b88985a07d1))
* Update engine for dependency downloading ([f051e7f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f051e7fe68b4c31190ceded92cbdab506d13f400))


### Fixes

* Adjust lookahead grammar ([1bcef05](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1bcef057ed98a2cbc48bdbb8d634c11cf9cd5331))
* Almost infinite loop while checking dependency cycles ([c5ac21e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c5ac21e546538f8fff945c9f8fe04c3092bcb4c7))
* Attribute references in nominal values are not checked properly ([560f499](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/560f499c9ddea289bf96bd019c7298f2dea621c3))
* Better handling of Unicode Supplementary Planes ([75cbcd6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/75cbcd60a02fbe2ff7383bc6d17f7c88204a5a2f))
* Bridge for Git configuration files may be ignored by the macro tracer ([98da01d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/98da01d23d86c15c7c7dfc060c2c2c69baef18f4))
* CICS status codes substitutions are not always performed ([bdde1de](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/bdde1de05bd4f189323efb5b2d887e1b67e4357e))
* Dependency collector improvements ([7dfc2f0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/7dfc2f01100384975887e48b7588270101329291))
* Disable unused antlr features ([917cf3d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/917cf3d7e0a54c5bdcba72a0c1935c985fd4fe67))
* Enhanced multiline support in TextMate grammar for source files ([aa7dc9c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/aa7dc9cbb6988f3ea1c201fe582c270d78452b3e))
* Improve label parsing accuracy and performance ([6e50e1b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6e50e1bbcc4a6044050a8fc3768a337a2522aff3))
* Long URIs may cause high CPU usage ([f0536df](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f0536df198f1ec189c6400e1e657e3108327d986))
* Miscellaneous performance enhancements ([e06c31d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e06c31d0d08691b304d368f4de203ffab1c34315))
* Normalize B4G program names ([db46aa7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/db46aa7f968955e0bfdafb03b8309532f581baf6))
* Redudant id_index formation ([504a200](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/504a200221892424e66130d96591d3d06d5af381))
* Replace configuration pop-ups with CodeLens ([c25906a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c25906a4e7311b9caa89ab81e4c2131f4ec280f9))
* Source code might not be reparsed when dependency name or location changes ([33c239b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/33c239b0d8c48cae911d9266fd2990115abc5765))

## [1.8.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.7.0...1.8.0) (2023-05-24)


### Features

* Cache contents of remote files ([db60a63](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/db60a636ee92ff7c6fef8a10ab7c0418cf7c668d))
* Infrastructure for fetching of remote files ([3650d48](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3650d48ac3753b88ffead8cf7f02c080a3e930d2))
* Non-UTF-8 encoding detection ([65ed2f7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/65ed2f7bc6ffb74cbb515e89d492cfe5c1e43290))
* Support on-demand dependency retrieval via FTP ([742e398](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/742e398713b3266e0ed6ea86057c334e2d152016))


### Fixes

* Adjust extension activation event ([4abf755](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4abf7551f7423b9f96abda41ccc66a3825b97340))
* Best-effort support for setting breakpoints in non-filesystem documents ([26ebd94](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/26ebd9406271e8e8a273cf8e863c01ae736e3519))
* Closing a dependency without saving does not trigger reparsing ([8b576fa](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/8b576fab7da69e83b03aab1713d89715052f711d))
* Configuration request sent before initialization is done ([a811a3b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a811a3b1b1e6fa8600734f30f4c5e4500641ff82))
* **deps:** Broken ZIP extraction in VSCode test ([1582f04](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1582f04cad3d5952fda89425bc50e15041eb95a9))
* High CPU usage while going to the symbol definition ([ef737c8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ef737c858c23d55a5bc280053949e693d434bf84))
* Implicit *NOPROC* processor group ([2aa82d1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2aa82d14e7b102b8184db127132d2e04d3154090))
* Inconsistent identification of inactive statements ([0e18699](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0e18699edae834f93c0e4d1d153a69018291f018))
* New or changed files are parsed only when they are opened in the editor ([c52e4f7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c52e4f77939e01627b61a34f48f1eb03b063f9d0))
* Notify diagnostic consumers after document close ([5bf404b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5bf404bae34ec1ccdecd3ac39aecb182cb9c2f69))
* Only refresh libraries that are caching content ([2d7a98a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2d7a98a0fcc9af2543bf6b0d79de5212e3f21b6a))
* Operands no longer classified as remarks when instruction or macro is not recognized ([189fd2d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/189fd2d1d0a3a448775ac75fea8d5d17c04f39bb))
* Prevent pgm_conf not found prompt ([d57895e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d57895efa236fbd3c7b6f1f96146377e61a1ed18))
* Source code not reparsed after missing dependency is provided ([fbdd27d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/fbdd27db95b1dfb3b60bf4a53b71d295ff9a89dd))
* The parsing library may crash when the external library parsing is actually asynchronous ([471a8ad](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/471a8ad510207ad60e24639fbc9364c4ae77c689))
* VSCode enters an infinite loop of opening and closing files ([6b92479](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6b92479d6c29d5ff775924a108ed34d35124c70c))


### Other changes

* Asyncify handling of requests in the language server ([a57504c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a57504c65687b627d9f3a261e4b8f26633789239))
* Coverage issues ([ba12f50](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ba12f5009a84bad7d215b8755d7aeea2be92d586))
* Do not keep analyzers, only the results ([d68f492](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d68f492b09ac1151504e9a1b8ef736d5a3122b48))
* Enable typescript strict mode ([d98025a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d98025a387b4f3e425372e9d850748c2bb78fd21))
* File manager responsibilities ([166ecd0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/166ecd0ba29601e336d82e94e19cbf2caa29fe7f))
* FTP Connection pooling ([baa4076](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/baa40765374de5fcb2bf4daaf9c4bb813fda3b13))
* Patch semantic release rate limit issues ([2a5db19](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2a5db197c30b409787246a93fccd98d744486f42))
* Reorganize handling of requests and file parsing ([33895b8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/33895b8712a84e87abd826f3879f66e57801c208))
* Task control transfer enhancements ([3d1b562](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3d1b56275b6b24114be681c391c16d43589670cf))
* Task yield ([5267118](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5267118d2501235e37346eded1586ddfb914ff89))
* Update workflow permissions ([0f59395](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0f593952f35541ae29e2a6ca7b7b25450bdb344f))

## [1.7.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.6.0...1.7.0) (2023-03-08)


### Features

* Add support for CICS BMS files ([ef158c1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ef158c147530cd7ea9bb8df7e67689f53fbe020b))
* Best-effort navigation for instructions in non-executed macro statements ([1c41380](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1c41380ab5c458e366d524ef3f75892377cb8eeb))
* Fading of inactive lines ([0ccd11d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0ccd11d675fd3f4dc4c2b76a7391ffbc90cf1353))
* Fading of preprocessor statements ([c7ca666](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c7ca66606a0e99f8bd5277039ab3adb21c41a034))
* Provide more details about machine instructions in hover texts ([48ed148](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/48ed14888754d9363f280b023a88278ead476969))


### Fixes

* Language client used before ready ([a067495](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a0674955de31d1d1d694c74fd95f3516b79ad18e))
* Large macro documentation is not highlighted correctly in hover texts ([3179b14](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3179b14b67505444a18600c23e0025270541e1ca))
* Patch URI parsing library ([741ef11](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/741ef11c894c2e719c129e2289a90f6ab71d778c))


### Other changes

* Always build fuzzer source code ([5bca92b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5bca92b23cdc298ebc9728a12e3dd0969f97fafb))
* Event based macro tracer tests ([67913e6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/67913e6a70e64b7273e73fb9769154738687ac44))
* Generalize logical line ([38d0bcc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/38d0bcc892608f4dcd4edef38a905631e5a269fb))
* Move java dependent build steps ([f160a17](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f160a17b2442d490c79e1f77d1d1aeff1604f947))
* Outline for symbols generated by macros ([4536842](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4536842a998e43a6ab6fbe154e4414d3031009c5))
* Remove the internal thread from the macro debugger ([0ea94b8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0ea94b873b9b42b248095afeb2e8d88ec385dbfb))
* Support asynchronous processing in preprocessors ([2539921](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/25399216816cb5ffa84e02682db93d9eabd8e24d))
* Support non-blocking step-by-step parsing of dependencies ([b5f1cfc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b5f1cfca8b5b013ab01a66192200460353cef89d))

## [1.6.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.5.0...1.6.0) (2023-01-18)


### Features

* CICS preprocessor statements highlighting and parsing ([3fd716b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3fd716b9387d56c492ce1336d90f5fa1d2388d60))
* Code actions for an unknown operation code ([f3522fd](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f3522fd7f5a034a2db46ecde42c3ab7c72577c44))
* Command for downloading copybooks allows selections of data sets which should be downloaded ([44a19d9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/44a19d915019a49457f881c6c6b2ea34d49785b6))
* DB2 preprocessor statements highlighting and parsing ([73b545f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/73b545f942908dbb841493f3cefab398b07c7c8c))
* Endevor preprocessor statements highlighting and parsing ([cb1a834](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/cb1a834467f9a27c166042f0915122e57a01d90b))
* Implement step out support in the macro tracer ([e0529a8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e0529a873eabeb236e2c85189af43afdc0120be2))
* Instruction suggestions are included in the completion list ([17a6eff](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/17a6effa59597ccdcdba82ff39d9d83bfb243658))
* Quick fixes for typos in instruction and macro names added to the code actions ([aaa6703](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/aaa6703500061bc42d5242af663da6b8560bacf3))
* Support for the SYSCLOCK system variable ([6116699](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/61166994698a9764ccb19823d6708288f0e79efc))


### Other changes

* Abstract id_index pointer nature ([cdb8a9e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/cdb8a9e8e1aec847144cbe49ed549c9f40fc9dcd))
* Enhance coverage reporting ([87dcc26](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/87dcc26cd4b930e0c247c3d7a69bae22e1b512f2))
* Keep small IDs inline in id_index ([e871d92](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e871d926240c788ee64afdc6851ee3e2093376ed))
* preprocessor copy members ([e4da1ef](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e4da1ef8515e7f43a22a83f1496212af04ef298a))
* Reference matching version of resources in the readme file ([3f13c0f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3f13c0f67b91b18640c84170d3981c500afdec20))
* Remove unsupported actions ([f2eee14](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f2eee14850ca7499567e3657d27e98d068a30b21))
* Update GH actions due to node12 deprecation ([e912d22](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e912d22272d95054491d584718312dd8ba0c7c9f))
* Upgrade all the things ([ce8ce03](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ce8ce03ba2780596f7d7d24726f22e8cf274dafb))
* Workflow refactoring ([a3387a2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a3387a2f6ff11053d82fabf13edd8c049b449d07))


### Fixes

* Do not display messages related to Bridge for Git configuration unless it is actively utilized ([1c8fb3e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1c8fb3ef3e17e4441d90f58d3aca7b0e4f831774))
* Enhance language server response times ([3452dfd](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3452dfda927297fadada8697f0ff6d65d102b328))
* File extensions are now ignored when  option is not provided in proc_grps.json ([40862d5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/40862d5384177a54f368cb8933c76db35597288d))
* Language server may crash on hover ([36c8895](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/36c88955dfc8b2233f6b02b6ec4fdd2d35ffd77a))
* Language server may crash while generating the document outline ([f85ae1d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f85ae1d0e08b4eb98f303dcf654bf926c0923b8d))
* Library contents are now shared between processor groups ([25dfa69](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/25dfa690f2f7e5923f61669ad5a7be08a8675350))
* LSP requests on virtual files are evaluated in the correct workspace context ([64420d1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/64420d170a21d20e2d344b7631dfd7fbeff92fa0))
* Macro label is the preferred go to definition target unless the request is made from the label itself ([1fb6427](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1fb6427b23cd60c12c479bd70dcf78eccd41aca4))
* Missing references and hover text in model statements ([a81918a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a81918a7c5f83324d9d0c9f733651f93820d8784))
* Regex workaround ([9a6a2cb](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9a6a2cbd0657e796b3fcc2fe9d9d0b886e58e965))
* Restrict configuration file patterns ([9938bc2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9938bc290afc88916f19e65c5c94f730c0fb621e))
* Sequence symbol location is incorrect when discovered in the lookahead mode ([20dda28](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/20dda28571e1ef437230a732c013479f8a6e740a))
* Sort variables in the macro tracer ([94b3acd](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/94b3acdd4cce0bd51fe79585ad7151f68abf41a2))
* The language server may occasionally work with obsolete configuration ([8d39b9f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/8d39b9fae4f6c279e1aa4a03432d5161b0f2dd72))

## [1.5.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.4.0...1.5.0) (2022-11-02)


### Features

* Add support for SYSM_SEV and SYSM_HSEV system variables ([a2760c5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a2760c5918ac178246b00f241279856d05b85e80))
* Add support for the SYSASM variable ([bb6e4e0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/bb6e4e0603dd97cf46365ac6b410631196b388d0))
* Add support for z16 instructions ([af9f01f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/af9f01f82fb447c78106ad43699ff3a9d7ef9b27))
* Endevor Bridge for Git integration ([bda2fbc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/bda2fbc6f6fc1c9e20e5d801a593947d89c0f446))
* Line and block comment handling support ([fb0cf41](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/fb0cf41f4f0a4443585bd8028713b80cd2f55eca))
* Support O and T attributes in machine expressions ([2ab7dde](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2ab7dde27468e33e62fc8e9d701c05f551db06f9))


### Other changes

* use span in ca variable functions ([a7b4880](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a7b4880dd7f50c727156497786e775c4bd58e9d8))
* Variable concatenation ([a4741d0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a4741d0bb64e245a5f927a3bd7dd6818c367f3c3))


### Fixes

* CA function unary operator ([6cc773f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6cc773f5227912f6e5e41acd994164c0f61eb1be))
* DB2 preprocessor produces more accurate output ([072dca6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/072dca64bc4d2956066c7c7c975fa0f1cca80a10))
* Different GBL types of the same variable detection ([c25b963](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c25b963d9661b22b7d410ef6a107cf4757feb85d))
* Evaluation of O attribute must use the instruction set available at the point of use ([71d85d3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/71d85d36e34fc4de8754351e459018e8c4795e48))
* Evaluation of subscripted expressions in CA statements ([37f6235](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/37f6235d04c11c2d73d7b3ae8ee603820e882e9c))
* False positive sequence symbol redefinition diagnostics generated ([211a860](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/211a860319f60542bdfd0721590768ba87dec9af))
* Incorrect evaluation of the T attribute in EQU statement ([72dad69](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/72dad691a818b2c25775198026ced4321d3b3f56))
* Incorrect implicit length computed for nominal values containing multibyte UTF-8 characters ([35d6d0b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/35d6d0b5ff9848147016d507ac3f325f0cbdb925))
* Incorrect ORG instruction behavior when operands contain EQUs ([e728819](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e728819f17da05c4279f6710b0e6eb9b6b121d54))
* Incorrect parsing of multiline operands in macros ([a255e0a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a255e0a6717ff3519ce8054f834e4e2641744d3e))
* Invalid substring may be generated when conditional assembly string contains multibyte UTF-8 characters ([af0eeb5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/af0eeb5796eb61e9b2238f1854fa74b5ba2e560f))
* Library diagnostics point to a respective configuration file ([7c7d7a0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/7c7d7a07360c8245cf5732c6c1be5e9ae40983ad))
* Missing special case handling when triggering the lookahead mode ([465985c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/465985ce832a69ca81e855284c17ce4d2d8b632e))
* Produce only a warning when immediate operand overflows in machine instructions ([0b5806e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0b5806e56233cedf8f503669386dc25104d741b5))
* Relax PROCESS operand validation ([466e690](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/466e690a621ba9e0d0491ad8abe94f2266d5a7bd))
* Self-defining terms are incorrectly parsed in machine expressions ([886c2c0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/886c2c07d386f7223fcf191c66d71c9a51d2b0a6))
* Syntax error while parsing the END language operands ([c6b8b97](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c6b8b976cc00af313735b13251018f0e435f13d5))
* T attribute of a USING label may be incorrect when the label is mentioned in the macro name field ([2015913](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/201591332b51ef157471c269db17e681794825b9))
* TITLE instruction name field support ([1df0fa2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1df0fa2478a4ad80ef52473f2f6b3c216194bb5e))
* Utilize alignment information during dependency evaluation ([fd9d73a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/fd9d73a4d7d959cf4c0a180f7428d29d00167af6))

## [1.4.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.3.0...1.4.0) (2022-08-30)


### Features

* Code completion for machine instructions and mnemonics utilizes snippets ([b17bc3a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b17bc3aab459b5deb294b6c9f4a1bd8824064deb))
* Copybook download command ([f383f68](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f383f685e6a43fdd4987165d36adc2fc085de3cd))
* Preprocessor for expanding Endevor includes ([5e542e4](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5e542e46b5ff146976058a08ee21f2d356dc81a5))
* Support for multiple preprocessors ([6029009](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/60290095469b827ae424048b302965914742ded5))


### Other changes

* **ci:** Reorganize build and check jobs ([f47ea12](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f47ea1265a63550ef61aed43c689c0c2ec81fc3a))
* Handling of UTF-8 chars in URLs ([4dc5177](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4dc51776725b86cd09da2602e65a18824ef7d870))
* Minor doc update and typos ([62d8287](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/62d82874d035127c723c25819a533b29d0d462d9))
* ORG instruction ([25761e8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/25761e85f1980aac43499b787164264dd4b67f82))
* Sonar workflow update for prev job failures ([11c38fe](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/11c38fe63e1c077dd08d0c6e19626eaaf8b8976d))
* Update llvm brew ref ([29ced13](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/29ced13073d72945e47648442cbae7336cfca91a))
* Update README.md ([a8536d7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a8536d7a05a698bd77ceff2b25117c5e01b71dc8))


### Fixes

* False positive diagnostics generated for statements included by the COPY instruction ([fb95c5e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/fb95c5e7e136cc6b062382e2d1ddc4ae783e79f1))
* Improve CICS preprocessor accuracy ([f59d218](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f59d2183b7b786a548f88a0698cefe7fa3a86d83))
* Improve parsing performance of CA operands ([dbcdcff](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/dbcdcff2f3128c529ae3c2d169b83570daa80003))
* Improve performance of dependency tracking and expression evaluation ([f1fcc19](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f1fcc198cfa5ad8146c6e79d76f2052bc6ec3551))
* Improve performance of file system event processing ([0a39937](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0a399375420a5c5a5ece295a6356b0d352ac6ded))
* Improve performance of generating document outline ([c4695da](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c4695da23477a5cf747da99330068f3a3dd9b7a8))
* Invalid CA variable definition may crash the language server ([70516e4](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/70516e4f05810822a71fa3dd4fef7cc14dbf93fd))
* Reduce memory footprint ([34e37cc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/34e37cc8bd9bc09e30acb5a63fe173718615e3ee))
* Sequence symbol redefinition diagnostic issued even for symbols excluded by CA statements ([e94f03d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e94f03ddca2c88cae9c7758cd0e203eaee68f279))
* The language server may crash when a line is continued by the preprocessor output ([0bda7f1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0bda7f158a075f864a65998390c0626dd873a3e9))
* Validation of mnemonics with optional operands produced incorrect diagnostics ([947cb40](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/947cb4049b19cec8d52690c84ea316867240a03d))

## [1.3.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.2.0...1.3.0) (2022-06-30)


### Features

* CICS preprocessor now recognizes DFHVALUE constants ([a13e10d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a13e10dd9525f383fca222bff6693749df492ab0))
* Conditional DB2 preprocessor ([2b8e38b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2b8e38b185e426335635f2caa586d3dd20006b7e))
* CXD instruction support ([19466b4](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/19466b40f680a1b90085dab7f2890285da987847))
* Enhanced commands for continuation handling and trimming of oversized lines ([675256e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/675256e3161876c97f0dd7b49c0ec4726f18ce11))
* Home directory substitution is now supported in proc_grps.json and pgm_conf.json ([83271c5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/83271c50ff877104eec32430147dc265977112f8)), closes [eclipse-che4z/che-che4z-lsp-for-hlasm#221](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/221)
* Machine option + JSON synonyms validation ([ea71391](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ea71391dfd37c53ad5658fd19119ecf78613e47b))
* Provide the name of a missing variable or an ordinary symbol in diagnostics ([9ba5294](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9ba5294f5835b1371dd9595c21ffb22f7b2c49ea)), closes [eclipse-che4z/che-che4z-lsp-for-hlasm#265](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/265)
* Show hexadecimal offsets and lengths in hover texts ([1a5e8a3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1a5e8a334e3430539a58e2c3ba583eccfafe1356))
* Support for SYSVER system variable ([1af16d0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1af16d0c60eaa874172854390f2684cd06c79003))
* SYSIN_DSN and SYSIN_MEMBER support ([dde525d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/dde525db8e6b6c5f5b40868a346b840ed9fdfc17))
* Visual Studio Code workspace variables can be used in pgm_conf.json and proc_grps.json ([e396b17](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e396b174ca1d29526b38fa5f18a763a439f35b08))


### Fixes

* BASSM has the RR instruction format ([5304035](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/530403574434b542766d1f2bd5a6c6926ace6ff6))
* Changes in configuration files were not detected on Linux ([ebc1ae8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ebc1ae8b1a9462e8f9856b99f3b1966a256a39b4))
* DB2 preprocessor incorrectly processes line continuations from included members ([0acf426](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0acf426e2e3b524801bccf53b8ee452f803d3200))
* Incorrect attribute parsing in CA expressions without spaces between individual terms ([821fae0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/821fae08ee83f0c301c2685afacf3320ce9f62fe)), closes [eclipse-che4z/che-che4z-lsp-for-hlasm#263](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/263)
* Incorrect attribute values generated when literals are substituted in CA expressions ([861c09c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/861c09cf890b79f5292ad7069608cde7fa9d35b8))
* Incorrect remark parsing in CA statements ([4e4a516](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4e4a516feafb41a954c5c0e70cb6ebb2863cbb80))
* Language server may crash while processing an unexpected operand ([dd714da](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/dd714da2d4b30a8a054addf8ccb0c45efec688aa))
* Parsing of numeric nominal values must be case insensitive ([3b05e1c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3b05e1c2536f2979697dd6bc21f43593ec9483df))
* Partially resolved value used as the final value by EQU statement ([0899cf9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0899cf9e40a144a9f13c11bbd5e6848fa1207acf))
* References to CA variables in strings are not reported ([3881b55](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3881b552bc4dd624e8991688dff662c4abe1a8e8))
* Remove deprecated property from the default configuration file ([790e128](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/790e128625350a7dd1e74a5f7227047e4ccec3aa))
* Return correct variable type for values provided in the macro's name field ([39f7ce2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/39f7ce2e10504ffb573ca611604591a423a0d192))
* Revise machine instructions ([be53133](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/be53133ad15b4e276ab810a982bc335e679e1896))
* String functions are not recognized in concatenations ([79456d5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/79456d53e79e4aa9d2a9339489bee130aa2a2cac))
* Structured macro variables were not forwarded correctly when a dot separator was used in the macro operand ([dcb5aeb](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/dcb5aebf87d4c5a6928c702f46ed912b83cb460b))
* The language server may crash after reloading the configuration ([860d453](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/860d4532a934306640e0e4495c2ae7e6222f1a56)), closes [eclipse-che4z/che-che4z-lsp-for-hlasm#271](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/271)
* The language server may crash when a complex expression is used as a variable symbol index ([e49745f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e49745f26621165f9a233b1b0ab6a7108078164e))


### Other changes

* AINSERT grammar tests improvements ([e2fa7d8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e2fa7d88ca908208dbefe62bf91fa8583c8159a8))
* Preprocessors ([7ea7ce8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/7ea7ce80dfa2ff3eecf6c898fee478c0b81fdf68))
* Reduce header dependencies ([2a7f8cb](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2a7f8cb640884201e3911979961252e7104c10fd))
* Replace internal path uses by URIs ([2c1aab6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2c1aab6d3a14fb26b2f7138ed4af55fd43e06bb4))
* Replace internal path uses by URIs [#2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/2) ([c2d5593](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c2d55930e432f655aea0fdb0d2648752859e51d2))
* URI representation ([1f664c2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1f664c29550f8b8551af317073f891d04953c932))

## [1.2.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.1.0...1.2.0) (2022-05-11)


### Features

* Allow viewing content generated by AINSERT instruction and preprocessors ([#248](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/248)) ([5313215](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5313215ebfd120856c49c410212da4f9919f93d0))
* Assembler options can be specified in pgm_conf.json to override values from proc_grps.json ([bf24968](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/bf24968e1d62b5c888ec82e3b72fba55421bb0f4))
* Basic GOFF, XOBJECT and SYSOPT_XOBJECT support ([3e6daed](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3e6daedc53fcda956bf59ae5e9a3a36d82db643e))
* DB2 preprocessor VERSION option ([#255](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/255)) ([2c032cc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2c032cc20bf82f0ea281063b5d66ca459da9ee0f)), closes [#229](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/229)
* Expand the list of associated file extensions ([#256](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/256)) ([3864bd9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3864bd9b93164a62be126a0b1eb5ef17cb583ec7))
* Instruction set versioning support ([f9532c5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f9532c54b194261b9e67d46760d91ab93bbca579))
* MNOTE support ([7ac224c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/7ac224cbec1337defcf85f36e75299f1003fe408))


### Other changes

* Added missing tests from Rally ([6b59233](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6b59233475e2112ac61fb4ee569916428b5b5d18))


### Fixes

* Double lock while closing files ([#254](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/254)) ([992405e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/992405e8c8f07082d65da1a916cf0ca691918d45))
* Empty arrays now behave similarly to other subscripted variables in the macro tracer ([ce0ba21](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ce0ba21b97de4965b90cdd3dc8ff098ea6007c20))
* Empty operands ignored by the SETx instructions ([5d13a69](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5d13a6987a1d189a56874d3016732d3b35b1c540))
* Forward structured macro parameters correctly ([89d2936](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/89d2936c35534474ad17075794e5de3fea289796))
* Improve detection of HLASM files ([21521e4](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/21521e49f1aee96b1d5e1187e3f96c9ceb81cd03))
* Incomplete conditional assembly expression evaluation ([08c5429](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/08c54290bd3520e9f7a8f145b020840b90aef623))
* Incorrect operator precedence in conditional assembly expressions ([0d9764e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0d9764e9e01d65b34f9513956f52494077f28c25)), closes [eclipse-che4z/che-che4z-lsp-for-hlasm#259](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/259)
* Lifetime issues in the file manager ([#257](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/257)) ([c3d5d90](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c3d5d906c29c3246056aa5669d00815818555cfa))
* N' attribute and subscript array contents ([#249](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/249)) ([31cc66f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/31cc66f60bc3a1503bd55730c877d964a325f574)), closes [#231](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/231)
* Parsing of negative numbers in machine expressions ([fee9e33](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/fee9e334aee7050e551a8c8c1f82a6a3df930498))
* Reaching internal ACTR limit now only generates warnings ([a223b48](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a223b48e1cd19df117e4b18c297036b51572b4e7))

## [1.1.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/1.0.0...1.1.0) (2022-03-29)


### Features

* Added support for SYSSTMT ([#234](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/234)) ([2d3da68](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2d3da6819ae9aa1167120815e3168cfbf76773f0)), closes [#213](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/213)
* Instruction operand checking utilizes USING map ([#242](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/242)) ([5e77dac](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5e77dacdbfa3c171b4f7a9e964703ccca3fec168))
* USING and DROP support ([#230](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/230)) ([f9dfd05](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f9dfd05bee2d344241ea92ec0ba4879dd93e3710))


### Other changes

* Expressions should not collect diagnostics ([#235](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/235)) ([45c2711](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/45c271103e1740710abf014ff2e7338a7a2bd2e3))
* Reduce data definition length evaluation dependencies ([#239](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/239)) ([b07419f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b07419ff31e2d8c8b05acb28e440cddeb35e1646))
* Use string_view in instructions and diagnostics ([#226](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/226)) ([0652b66](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0652b66cc5b366fec3a1f787f60f5ab26a869600))


### Fixes

* AINSERT immediate variable evaluation ([#243](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/243)) ([e8006e9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e8006e9b4c8e1ba262781e6e2a10e8296913b254)), closes [#236](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/236)
* Cross-section relative immediate references should be only warned upon ([#240](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/240)) ([fe243de](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/fe243def88410643af5d4060b5959eafab527dc7))
* Document navigation does not work correctly in Untitled documents ([#247](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/247)) ([4316546](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4316546c5a56c1c1de729b07444042a8a65de4ba))
* Incorrect module layout computed when ORG instruction is used in sections with multiple location counters ([#241](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/241)) ([e5838c5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e5838c5e026d85a178111f95895336e7202dd302))
* Single char strings highlighting ([#246](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/246)) ([37cd2d7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/37cd2d71cc936d07a93fc0e663d74a1c47c65ccd))
* System variables with subscripts ([#233](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/233)) ([f01e81b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f01e81b7656661104c9f68e0e6819fc116e01c64)), closes [#212](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/212)

## [1.0.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.15.1...1.0.0) (2022-01-31)


### ⚠ BREAKING CHANGES

* Release V1

### Features

* Literal support ([#207](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/207)) ([44a93f2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/44a93f28a124ba873811f0a5f314e02c1693655f)), closes [#193](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/193) [#75](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/75)
* Location counter length attribute support ([#208](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/208)) ([c2d64c9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c2d64c9023ce438af7e3737ced4330a9f5f805bb)), closes [#71](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/71)
* Toleration of EXEC CICS statements and related preprocessing ([#219](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/219)) ([5e10293](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5e10293594d96dcd64446de49c68a83e98a13ca9))


### Fixes

* Data definition grammar is too greedy ([#223](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/223)) ([dd9b557](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/dd9b55773cb887c6e538872529fbcf5e408f6dca))
* Improve statement parsing performance ([#201](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/201)) ([c949784](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c949784a87fb4b48dfdd6c37e54c4e205711da9d))
* Incorrect module layout generated when data definition operands have different alignments ([#210](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/210)) ([8660232](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/8660232500d8ce38ea1f8d5fe02bac15c590fe2c))
* Provide semantic tokens through language client 7.0.0 ([#211](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/211)) ([42a830b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/42a830b52586b358f9ef73dbe72a41a6ce8d7357)), closes [#82](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/82)


* Release V1 ([312295a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/312295a53f9b30245dce3f4c07bf05ba4246625e))


### Other changes

* Readme update ([35800e5](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/35800e57e560323dd21d24d1a1fe171ea56f8de8))

### [0.15.1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.15.0...0.15.1) (2021-11-11)


### Fixes

* Improve outline performance for files with large number of labels ([#202](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/202)) ([16def4b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/16def4bb701049a031d7b66fb876daa382c3d4bd))
* Language server must tolerate extra arguments passed by vscode in wasm mode ([#203](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/203)) ([f7f4fa6](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f7f4fa6bc6e3555ed093ebf1fcc7f07ff1fad37c))


### Other changes

* Add changelog for v0.15.1 ([d86d752](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d86d752f36512604ecf930b561e402cc8dc77afb))

## [0.15.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.14.0...0.15.0) (2021-11-08)


### Features

* Allow wildcards in proc_grps.json library specification ([#172](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/172)) ([9157ef7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9157ef78e903c80e484eaa6ec6f6970bd4647093)), closes [#69](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/69)
* Document outline support ([9f65cd4](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9f65cd4f6a473a349639b49b9836b020ad507788))
* Implement SYSTEM_ID system variable ([#182](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/182)) ([caa6fd0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/caa6fd04b380a6735a1379ff77f04b25c99b4cc2))
* Support START and MHELP instructions ([#171](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/171)) ([f9f2fb2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f9f2fb2852a7feed5e1fdca4a771588b15758762))
* Telemetry reporting ([#187](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/187)) ([70445dd](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/70445ddd3db3d1a8344aa151c7651c5e802459aa))


### Other changes

* Add Support section to client readme ([#192](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/192)) ([c68270f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c68270f53cfd9f6bf15289d35d75bb8fa75ddb84))
* replace theia docker images that do not exist anymore ([#197](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/197)) ([ac5cb3d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ac5cb3df74f0a80d8cab481469255e251854f3f8))


### Fixes

* &SYSMAC should contain only the macro name (fixes eclipse-che4z/che-che4z-lsp-for-hlasm[#168](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/168)) ([4e2fddc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4e2fddce850712783d6cfb14091af21692076a53))
* AINSERT operand length validation ([#196](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/196)) ([03d62ad](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/03d62ad65d1ed4167f60f7e41c6319118955dca5))
* Apostrophe parsing in model statements (fixes eclipse-che4z/che-che4z-lsp-for-hlasm[#163](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/163)) ([#164](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/164)) ([94843c8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/94843c8a8facdc621058b00100129331da67e66a))
* Diagnostics lost during JSON serialization ([#170](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/170)) ([f272f7d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f272f7d9820488a9ecda52d909679f67507cf2b7))
* DOT operator in string concatenation is optional ([#190](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/190)) ([33d9ecf](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/33d9ecfd14b7a9418418c640d164f7c1f6c73fc0))
* Enhance conditional assembler expression parsing, evaluation and diagnostics ([#181](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/181)) ([40a2019](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/40a2019b59e8fdc240b65da9537145b87e524de1))
* File with extensions for other files should not be set to hlasm ([#173](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/173)) ([fc49775](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/fc497751149b72c2b2a318b618928adabcd18032))
* Fix HLASM Listing highlighting on lines with trimmed whitespace ([#199](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/199)) ([4262e27](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4262e27861eef1aa7ed87fd8bcac898e6b679bff))
* Incorrect relative immediate operand validation (fixes [#177](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/177)) ([614c86e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/614c86e844fa7ff47c86b9ceec52f484e0d672aa))
* Infinite loop during lookahead processing when model statement is located in copybook ([#189](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/189)) ([176de31](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/176de31a040fe67e59a8abcf2852a63a3a1af5c1))
* Language server crashes while trying to list inaccessible directory ([60db271](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/60db2715b7a5356a102d80bb22a3add834b00a53))
* Lookahead mode does not work correctly when triggered from AINSERTed code ([#175](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/175)) ([f7143c8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f7143c8326f5ddbdea46827e7ccd2a700c841a33))
* Operands of dynamically generated statements may be incorrectly parsed ([#185](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/185)) ([1a9127e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1a9127e509cd0cf4137a3bc4a6d92ab742cb616f))
* Preserve mixed-case labels on macro calls ([#165](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/165)) ([d8545fe](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d8545fee66cf2987d9cdf179529df4d7987d9c6e)), closes [eclipse-che4z/che-che4z-lsp-for-hlasm#155](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/155)
* Process END instruction ([#184](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/184)) ([2d5ad75](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2d5ad7593f78a50483d528568996a4e6af701581))
* Remove ALIAS operand parsing limitation ([#178](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/178)) ([480e602](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/480e602c30fc6cf0cddcd017ee4abe66d75fc043)), closes [#157](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/157)
* Various fixes ([#166](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/166)) ([200b769](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/200b769e5da0082be264971fdc4a916d63426211))
* Write the error name directly to method name of telemetry event ([#200](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/200)) ([6dd6b9f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6dd6b9f91cfb10791809c5e4b833a4e1a78e693c))

## [0.14.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.13.0...0.14.0) (2021-08-18)


### Features

* Implement the CCW* instructions ([#154](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/154)) ([f750bcc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/f750bccc051abea4b1141a71720df109f0a3a670))
* Minimal DB2 preprocessor emulator ([#140](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/140)) ([77275dd](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/77275dd91e8a8ba40ca8a60bd2d47435ccf39ff5))
* Support for complex SQL types ([#146](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/146)) ([3e85b98](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3e85b98b476eb0b66566a9e729d540fe8fbb8f36))


### Fixes

* AREAD/AINSERT support in macros called from copybooks ([#138](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/138)) ([bdc3718](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/bdc371894730eef021f08f8cadb45f07d622ba6e))
* DB2 LOB locator and file variables are not processed correctly ([20a6fba](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/20a6fba747ca19682806eeadeef1ced851b881ea))
* Dependency files caching ([#129](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/129)) ([2541b7a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2541b7a18070e10071b5586827659f6695d54755))
* EXLR flagged as error by plugin ([#121](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/121)) ([e097903](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e097903698dcafac65c3ccc01faea6a78e9cc85d))
* Inline macros overwriting external definition stored in macro cache ([#148](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/148)) ([93107b3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/93107b315298ce9b3f5652f28b8ad7a3b8a7dbf3))
* language server crashes while evaluating conditional assembly statements ([#139](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/139)) ([249e85d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/249e85d2da17caca532d8fadd8517f215a4dbfd4))
* Remove (no longer supported) experimental flags when running WASM server variant on V8 version 9 and newer. ([1cabd76](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1cabd76999fdffa7a8e700e943813c9f41ac3431))
* Syntax errors reported in bilingual macros ([#152](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/152)) ([a8b1201](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/a8b1201c7cddef5c9d5b5a00b6ae71d3a6e75641)), closes [#144](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/144)
* Various small fixes ([#149](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/149)) ([c1a6896](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c1a6896ed799efa91f9cc7ff10f5f5d6b7981de3)), closes [eclipse-che4z/che-che4z-lsp-for-hlasm#143](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/143) [eclipse-che4z/che-che4z-lsp-for-hlasm#142](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/142)
* Various small fixes ([#150](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/150)) ([36fdbda](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/36fdbda1ca93e5a3d4ca173f6253f2b04c6ed53f))


### Other changes

* changelog + readme for preprocessors ([70caf82](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/70caf8292ca64b82f94533de3b83002f42a20168))
* Introduce a  mock usable throughout the parser library tests ([#145](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/145)) ([547948e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/547948e1a82f701338aa42de57c7f4e18f58c408))
* Parser reports all its diagnostics using single channel ([#141](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/141)) ([656caf3](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/656caf3a21d231713b7d66a8cdbf26091a4a20ac))
* Split complex classes ([#134](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/134)) ([78ca7e7](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/78ca7e75189df5123379110b2e1bb20b7552143e))

## [0.13.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.12.0...0.13.0) (2021-06-01)


### Features

* AREAD support ([#125](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/125)) ([052c844](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/052c84463cdd2bb390877ae682abca385a4c05ca))
* Provide users ability to use compiler option SYSPARM ([#108](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/108)) ([ccb3a0a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ccb3a0a38ef562cbee9ae2348ef278d352ced74a))
* Support for macro file extensions ([#117](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/117)) ([d5b21d2](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d5b21d2f37b815da3a6bc9dfaa3da3ee5ad87c68))
* Support missing instruction ([#113](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/113)) ([ec547cf](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ec547cf29a071167e6acffed3eac1900daf8df66))
* Support running under Node.js ([#111](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/111)) ([dc0c47b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/dc0c47bd756e293c0724c6e4a25b85a632aa3f46))
* UI enhancement for the WASM server variant ([#120](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/120)) ([2d73b0d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2d73b0db89b522e8b53fc39b0ed1fc526edd5fe3)), closes [eclipse-che4z/che-che4z-lsp-for-hlasm#122](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/122)


### Fixes

* conversion of an empty string to a number ([#119](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/119)) ([b5e6989](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b5e69896f9daed795e3c0ea62c89dd5eeec3a1bd))
* Fix crash when hovering over non-existing instruction ([3fbb22e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/3fbb22ee1837fc38ef2709cb5252c447b67e60b2))
* Refactor the way of collecting LSP information ([#110](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/110)) ([d767b6d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/d767b6d2860fbe90e25d9d5e9e876a1fa8fff4cd))


### Other changes

* Coverity scan ([#123](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/123)) ([daa662e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/daa662ee8e02f1e5fd41f912c2ab099225faee1b))
* DAP via LSP messages ([#109](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/109)) ([044acad](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/044acad3e5a57610d19492f79b90d759bb7d10d4))
* Implement suggestions from tools ([#118](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/118)) ([b6f231b](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/b6f231b01a8862eb415f6f00a2105fccc5a78934))
* Remove mention of unimplemented PROFILE option ([5f4ce87](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/5f4ce87e653e700331455c04c9307297d633910a))
* Statement refactor ([#93](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/93)) ([0f41701](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0f41701b3a23405d0e4e63f66700efe2eac22c46))

## [0.12.0](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/0.11.1...0.12.0) (2020-12-18)


### Features

* Diagnostics suppression for files with no pgm_conf configuration ([#89](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/89)) ([c287ff1](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c287ff13e166edf7a191fbf5482a125648b8986a))


### Other changes

* Add example workspace ([#44](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/44)) ([774bb1d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/774bb1dde75bdaf6456a6bf0b40bceca76760053))
* Add thread sanitizer to CI ([#78](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/78)) ([225b74f](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/225b74f408a29b1288a4f394cd79b23b31dc5295))
* automatic release mechanism ([#53](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/53)) ([0abf1d8](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/0abf1d8a35d41c9095cd57135f789a0e7b4e5036))
* fix obtaining version in theia tests ([23594bb](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/23594bb86207fa70f67ab5bb3e3f8df63fa7722f))
* fix semantic-release not creating pre-release ([9c65e12](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9c65e12ae598463d994874fa88d5da9ecc511639))
* fix set-env command that is no longer supported ([#95](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/95)) ([c7d061c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/c7d061cfa0bab6e9c1715c0ec7b2d265f3ff3e71))
* Improve test coverage of Language Server component ([#61](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/61)) ([6c67153](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/6c671533c28e180ddc369b74966af5bbedcc0b1e))
* Integration tests ([#51](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/51)) ([4b3d153](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/4b3d1531014a2601848168df6d52af2eb7e5890c))
* Refactor of attribute lookahead processing ([#84](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/84)) ([ce8e59d](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/ce8e59df8415935be2e007459d62ecd23ee0adad))
* Server test deadlock fix ([#83](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/83)) ([9cc2dfc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/9cc2dfc33404671df2dea213e146f33ce6ee4e3b))
* TI review of wiki pages and updates to readme files ([#56](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/56)) ([e66ea4a](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/e66ea4a823485335ea58fda14abf024743b95852))
* Update link of join slack badge ([1ec6bfc](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1ec6bfc7fbf239a8474659c1362e8c3fc642b753))
* wiki inside repository ([#50](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/50)) ([1be2a95](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/1be2a9594e050b49f26c30ea9b3a625cd5942825))


### Fixes

* Conditional assembly expressions ([#65](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/65)) ([99c45ee](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/99c45ee0a45d4b54b4c043e79afb92b73edee935))
* False positives ([#86](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/86)) ([34f3a5e](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/34f3a5ec52e38f3d21471bfb49962e991d348f99))
* Fixed little things in suppression section ([#99](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/99)) ([9374153](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/937415380644e2f26f51ff2fd6b7aa4ab462da41))
* Instruction completion issue ([#64](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/64)) ([8f31888](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/8f3188848a56dd6a1e90aa1a2da107dbeed4e8ab))
* Lexer infinite loop fix ([#85](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/85)) ([027b9f9](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/027b9f99a28ad5a60c9b0f0ed9762a2d643f8610))
* Show ampersands in names of variable symbols and macro parameters in macro tracer ([#79](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/issues/79)) ([2c2338c](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/commit/2c2338c89fb1f7aeb34fdb3315cf102a66ad3ddd))

