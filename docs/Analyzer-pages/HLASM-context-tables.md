HLASM context tables (in code referred to as `hlasm_context`) are composed of tables and stacks that describe the state of the currently processed open-code. This structure is persistent between source files within an open-code. It is created in an analyzer and has the same lifespan.

The structure is composed of:

-   *Macro & Copy storage*  
Stores macro and copy definitions.

-   *ID storage*  
Stores symbol identifiers.

-   *Scope stack*  
Stores nested macro invocations and local variable symbols.

-   *Global variable symbol storage*  
Stores global variable symbols.

-   *Source stack*   
Stores nested source files.

-   *Processing stack*  
Stores processing stacks in a source file.

-   *LSP context*  
Stores structures for LSP requests.

-   *Ordinary assembly context*  
Encapsulates structures describing ordinary assembly.

### Macro Definition

HLASM context stores visited macro definitions in the *macro strorage*.

A macro definition is represented by:

-   *Macro identifier*  
Identifies the macro.

-   *Calling parameters*.  
These parameters are assigned a real value when the macro is called.

-   *Block of statements*. 
Represents the body of the macro.

-   *Block of copy nestings*. 
An array with a one-to-one relation with a block of statements. Each entry is a list of in-file locations that represents how often the statement is nested in COPY calls.

-   *Label storage*. 
The storage of sequence symbols that occur in the macro definition.

When a macro is called, a *macro invocation* object is created. It shares the content of the respective macro definition with an exception of calling parameters as they are assigned real values when passed with the call. Also, it contains an index to the top statement of the invocation.

The macro invocation is stored in the context’s *scope stack*.

### Scope Stack

This stack (the stack of `code_scope` objects) holds information about the scope of variable symbols. The scope changes when a macro is visited. The initial scope is the open-code.

The stack elements contain:

-   In-scope variable symbols.

-   In-scope sequence symbols.

-   Pointer to the macro invocation (NULL if in open-code).

-   Branch counter (for ACTR instruction).

### COPY

HLASM context stores visited COPY members in the *copy storage*.

The COPY member definition is much more simple than the macro definition as it does not hold any more semantic information than the sequence of statements (the definition itself).

When a copy is visited, a copy member invocation is created and pushed in the copy stack of the last entry of the *source stack*.

### Source Stack and Processing Stack

These stacks are responsible for the nests of opened files (source stack) and what they are opened for (processing stack). As the relation of source entry and processing entry is one-to-many, the information is stored in two arrays rather than one.

When [[statement processor]]s are changed (e.g. when a macro or copy definition is processed, or a lookahead is needed), this information is stored in the processing stack. If a new file is opened during this change then source stack is updated as well.

The source stack contains the following:

-   *Source file identifier*

-   *Copy stack* – the nest of copy calls active for the source file.

-   *Processed statement location* – data that locates the last processed statement in the source file.

The processing stack contains the *processing kind*.

The reason behind organizing these two stacks in such a way is:

1.  The context has enough information to fully reconstruct the statement.

2.  Ease of retrieval of the correct copy stack for a copy statement provider.

### ID Storage

ID storage holds the string identifiers that are used by the open-code. It stores the string and retrieves a pointer. It is guaranteed that if two different strings with the same value are passed to the storage, the resulting pointers are equal.

It simplifies work with IDs and saves space.

### Variable Symbols

In HLASM language, a variable symbol is a general term for symbols beginning with an ampersand. However, they can be separated into several structures that capture a common behavior:

-   *SET symbols* 

-   *System variables*

-   *Macro parameters*

They inherit the *variable symbol* as a common abstract ancestor. SET symbols are further divided into *SETA*, *SETB* and *SETC* symbols. Macro parameters are divided into *keyword* and *positional* parameters (see the picture below). They are stored in whichever storage determines their scope (global storage, scope stack, macro definition).

<img src="img/variable_arch.svg" alt="The inheritance of variable symbols." />

### LSP Context

The LSP context serves as the collection point for the data needed to answer LSP requests. It is part of the HLASM context to be able to pass on LSP data between different parsed files.

The [[LSP data collector]] stores its values inside the LSP context tables.

### Ordinary Assembly Context

The above described structures aimed to describe the high-level part of the language (code generation). As we move closer to the resulting object code of the source file, the describing structures get complicated. Therefore, HLASM context contains an object storing just this part of the processing.

<img src="img/ord_ctx_arch.svg" alt="The composition of ordinary assembly context" />

Ordinary assembly context consists of three main components (see the picture above):

1.  *Symbol storage*. Stores ordinary symbols.

2.  *Section storage*. Has the notion of all generated sections, each section containing its location counters.

3.  *Symbol dependency tables*. Contains yet unresolved dependencies between symbols prior to the currently processed instruction.

#### Symbol

This class represents HLASM ordinary symbols. Besides its identifier and location, the symbol contains the components *value* and *attributes*.

Value  
*Absolute* or *relocatable* values can be assigned. In addition, it can also be assigned an empty value stating that symbol is not yet defined.

Attributes  
Holds symbol attributes like type, length, scale and integer.

#### Section

This class is a structure representing a HLASM section (created by CSECT, DSECT, ...). It contains the *section kind*, which describes the type of the section prior to the used instruction. The structure also holds the *location counter storage* with defined location counters.

#### Location Counter

This structure contains data and operations for one location counter. The data is stored in the helper sub-structure *location counter data*.

The location counter data is a structure defining the current value of the location counter. It consists of:

-   *Storage*, stating the total number of bytes occupied by the location counter.

-   A vector of *spaces*, blocks of bytes with unknown length.

-   A vector of *storage* between each space.

-   The currently valid *alignment* (used when data contains spaces).

The location counter value is transformable into a relocatable value. It is represented by the structure *address*.

The address consists of:

-   An array of *bases*. A base is the beginning of a corresponding section. They serve as points of reference for the address.

-   An array of *spaces* that are present in the address.

-   The *offset* from the bases.

The common composition of an address is one base section (as the start of the address) and the value of storage (as the offset from it).

The need for the whole array of bases to be present is because addresses from different sections can be arbitrarily added or subtracted. This information is needed as the correct sequence of arithmetic operations can reduce the number of bases (even spaces) to zero and create an absolute value. This value can be later used in places where a relocatable value is forbidden.

A space is a block of bytes with an unknown length. It is created in the active location counter when execution of the counter’s operation cannot be performed due to undefined ordinary symbols. The table below lists the different kind of spaces and the reason for their creation.

| **Space Kind** |                                                  **Creation**|
|:---------------|-------------------------------------------------------------:|
| Ordinary       |            when an instruction outputs data of unknown length|
| LOCTR begin    |     when defining more than one location counter in a section|
| Alignment      |  when the current alignment is unknown due to previous spaces|
| LOCTR set      |    when moving the counter’s value to the address with spaces|
| LOCTR max      |when moving the counter’s value to the next available location|
| LOCTR unknown  |         when moving the counter’s value to an unknown address|

When a space length becomes known, all addresses containing the spaces need to be updated (remove the space and append the offset). Therefore, space structure contains an array of address listeners. When an address is assigned a relocatable value that contains the space, the address is added to its array. This serves as an easy point of space resolving.

The ORG instruction can arbitrarily move the location counter’s value forward and backward. In addition to that, ORG can also order the location counter to set its value to the next available value (the lowest untouched address, see [[HLASM overview#Location_Counter|HLASM overview]]). Combining this with the possible spaces creation, the location counter holds an array of location counter data to properly set the next available value.

#### Symbol Dependency Tables

HLASM forbids cyclic symbol definition. This component maintains dependencies between symbols and detects possible cycles. This section describes the main components of dependency resolution.

- **Dependant**  
A structure used in the symbol dependency tables. It encapsulates objects that can be dependent on another. A dependant object can be a *symbol*, *symbol attribute* or a *space*.

- **Dependable**  
An interface that is implemented by a class if its instance can contain dependencies. The interface has a method to retrieve a structure holding the respective *dependants*.

- **Resolvable**  
An interface that adds up to the dependable interface. It is implemented by objects that serve as values assignable to *dependants*. It provides methods to return a *symbol value* with the help of the dependency solver.

- **Dependency Solver**  
An interface that can return the value of a symbol providing its identifier. It is implemented by ordinary assembly context.

Having described the building blocks, we can move to the symbol dependency tables composition.

- **Dependency map**  
The primary storage of dependencies. It has *dependants* as keys and *resolvables* as values. The semantics for pair *(D,R)* is that D is dependent on the dependencies from R. Each time a new dependency is added, this map is searched for cycles.

- **Dependency Sources Map**  
    Serves as a source object storage of a resolvable in the dependency map. Hence for the pair *(D,R)* from the dependency map, the source object of *R* is in the dependency source map under the key *D*.

    The source objects are statements. As one statement can be a source for more distinct resolvables, this source map only stores pointers to the *postponed statements storage*.

- **Postponed Statements Storage**  
Holds statements that are sources of resolvables in dependency map. The reason they are stored is that they cannot be checked yet as they contain dependencies. Therefore, they are postponed in the storage until all of the dependencies are resolved. Then they are passed to the respective checker.
