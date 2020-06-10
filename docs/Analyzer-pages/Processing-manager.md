The processing manager is a major component in the processing of a HLASM source file. It decides which stream of statements is to be processed and how statements are to be processed. It contains components responsible for instruction interpretation as well as instruction format validation.

The nature of HLASM source interpretation requires that various parsers and code generators interlace to implement semantics of all instructions. The processing manager performs this by maintaining 2 sets of active generators (“providers”) and consumers (parsers, “processors”) and executing them on demand, in an interleaved manner.

<img src="img/processing_manager_arch.svg" alt="Architecture of the processing manager" />

### Overview

The following objects passed by the analyzer serve as an input for the processing manager:

-   *Parser* which provides statements from the processed file. Further on in this chapter, we will refer to the parser as the *opencode statement provider*.

-   *HLASM context tables* that hold the current state of the parsed code.

-   *Library data* defining the initial state of the manager (whether the file is a copy member, macro definition, etc. For more information, see [[#Initial State of Manager]] below.)

-   *Name* of the processed file.

-   *Parse library provider* to solve source file dependencies.

-   *Statement fields parser* for parsing yet unresolved statement fields.

-   *Processing tracer* for tracing processed statements (see [[macro tracer]]).

### Composition

As the processing of the HLASM source file is rather complicated, we define a complex set of abstraction objects over the complicated assembling of HLASM language:

- **[[Statement providers]]**
A statement provider is able to produce `statement` structures. Its functionality is to provide statements from its various statement sources (e.g. a source file for the opencode provider, a macro/copy definition for the macro/copy provider).

- **[[Statement processors]]**
A statement processor is an object that takes statement structures from a provider. Then, it performs a specific action with the acquired statement; namely, stores it into a macro/copy definition (*macro/copy processor*), looks for a sequence symbol (*lookahead processor*), or performs a contained instruction (*opencode processor*).

- **[[Instruction processors]]**
Instruction processors help opencode statement processor in performing actions with the instructions contained in a statement. Each one of four instruction processors (Macro, Assembler, Machine and Conditional Assembly IP) processes separate sub-set of a broad set of HLASM instructions.

- **[[Instruction format validation|Instruction format validators]]**
Instruction format validators are used by instruction processors. As an input, they take operands of an instruction and validate their correctness.

The processing manager encapsulates above mentioned objects and determines which processor/provider is used next.

### Main Loop of Manager

The processing manager contains an array of active statement processors and an array of active statement providers. It is in the control of which processor–provider pair currently operates.

The main processing loop works with the currently operating processor and provider. In the loop body, the statement provider provides the next statement for the statement processor, which processes it accordingly. The loop breaks when all processors finish working and none of them is active.

When the provider ends its statement stream or the processor finishes its work, it is replaced with another. The following rules apply:

1.  When a processor finishes its work, the next processor is selected from the array.

2.  When a provider finishes — before the next provider is selected from the array — the manager checks whether it triggers the termination of the current processor as well (see *terminal condition* in the table at the end of [[statement processors]]). If true, perform rule 1, otherwise the current processor stays active.

### Initial State of Manager

During initialization, the manager sets various statement providers and processors as a default. This is very important as it determines the way how the source is processed. The manager determines this from *library data* passed by the analyzer.

Library data contains a file name and an enumeration indicating the kind of the file that is being parsed — the *processing kind*.

The *ordinary* processing kind states that the file being processed is the main source file (in HLASM called opencode). It is the first file to be processed. With this information, the manager initializes all statement providers and *only* the opencode processor. This initial state is applied when the analyzer has owner semantics.

The *copy* and *macro* processing kinds state that the manager processes source code that contains copy or macro definition respectively. Hence, *only* a copy definition processor or a macro definition processor is initialized. Also, all statement providers but the macro statement provider are initialized, as no macros are visited or needed as a statement source when processing new source code. The library data is passed when the analyzer has reference semantics.
