Processing manager is a major component in the processing of a HLASM source file. It decides which stream of statements is to be processed and how statements are going to be processed. It contains components responsible for instruction interpretation as well as instruction format validation.

Nature of the HLASM source interpretation requires that various parsers and code generators interlace to implement semantics of all instructions. Processing manager performs this by maintaining 2 sets of active generators (“providers”) and consumers (parsers, “processors”) and executing them on demand, in an interleaved manner.

<img src="img/processing_manager_arch.svg" alt="The architecture of Processing manager" />

### Overview

Following objects passed by analyzer serve as an input for the processing manager:

-   *Parser* that provides statements from the processed file. Further on in this chapter, we will refer to the parser as to the *Opencode statement provider*.

-   *HLASM context tables* that hold current state of the parsed code.

-   *Library data* defining the initial state of the manager (whether the file is copy member, macro definition, etc.; see *Initial state of manager*).

-   *Name* of the processed file.

-   *Parse library provider* to solve source file dependencies.

-   *Statement fields parser* for parsing yet unresolved statement fields.

-   *Processing tracer* for tracing processed statements (see [[Macro tracer]]).

### Composition

As the processing of the HLASM source file is rather complicated, we define a complex set of abstraction objects over the complicated assembling of HLASM language:

- **[[Statement provider|Statement providers]]**
Statement provider is able to produce `statement` structures. Its functionality is to provide statements from its various statement sources (e.g., a source file for Opencode provider, a macro/copy definition for Macro/Copy provider).

- **[[Statement processor|Statement processors]]**
Statement processor is an object that takes statement structures from a provider. Then, it performs a specific action with the acquired statement; namely, stores it into macro/copy definition (*Macro/Copy processor*) or looks for sequence symbol (*Lookahed processor*) or performs contained instruction (*Opencode processor*).

- **[[Instruction processors]]**
Instruction processors help opencode statement processor in performing actions with the instructions contained in a statement. Each one of four instruction processors (Macro, Assembler, Machine and Conditional Assembly IP) processes separate sub-set of a broad set of HLASM instructions.

- **[[Instruction format validators|Instruction format validation]]**
Instruction format validators are used by instruction processors. As an input, they take operands of an instruction and serve to validate their correctness.

Processing manager encapsulates above mentioned objects and determines which processor/provider will be used next.

### The main loop of manager

Processing manager contains an array of active statement processors and an array of active statement providers. It is in the control of which processor–provider pair currently operates.

The main processing loop works with the currently operating processor and provider. In the loop body, statement provider provides next statement for statement processor that processes it accordingly. The loop breaks when all processors finish work and none of them is active.

When provider ends its statement stream or processor finishes its work, it is replaced with another. The following rules apply:

1.  When a processor finishes its work, the next processor is selected from the array.

2.  When a provider finishes — before the next provider is selected from the array — manager checks whether it triggers the termination of the current processor as well (see *terminal condition* in the table at the end of [[Statement processors]]). If true, perform rule 1, otherwise the current processor stays active.

### Initial state of manager

During initialization, the manager sets various statement providers and processors as a default. It is very important as it determines the way how the source is processed. The manager determines this from *library data* passed by analyzer.

Library data contain a file name and an enumeration indicating a kind of the file that is being parsed — *processing kind*.

*Ordinary* processing kind states that the file being processed is the main source file (in HLASM called open-code). It is the first file to be processed. With this information, manager initializes all statement providers and *only* opencode processor. This initial state is applied when analyzer has owner semantics.

*Copy* and *Macro* processing kinds state that manager will process source code that contains copy or macro definition respectively. Hence, *only* copy definition processor or macro definition processor is initialized. Also, all statement providers but the macro statement provider are initialized as no macros will be visited nor needed as a statement source when processing new source code. The library data is passed when analyzer has reference semantics.
