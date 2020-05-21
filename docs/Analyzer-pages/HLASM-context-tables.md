HLASM context tables (in code referred simply as `hlasm_context`) are composition of tables and stacks that describe the state of the currently processed open-code. This structure is persistent between source files within an open-code. It is created in an analyzer and has the same lifespan.

It is composed of:

-   *Macro & Copy storage* – stores macro and copy definition definitions.

-   *ID storage* – stores symbol identifiers.

-   *Scope stack* – stores nested macro invocations and local variable symbols.

-   *Global variable symbol storage* – stores global variable symbols.

-   *Source stack* – stores nested source files.

-   *Processing stack* – stores stack of processings in a source file.

-   *LSP context* – stores structures for LSP requests.

-   *Ordinary assembly context* – encapsulates structures describing Ordinary assembly.

### Macro definition

HLASM context stores visited macro definitions in the *macro strorage*.

Macro definition is represented by:

-   *Macro identifier*. It identifies the macro.

-   *Calling parameters*. They are assigned real value when the macro is called.

-   *Block of statement*. It represents the body of the macro.

-   *Block of copy nestings*. It is an array with one-to-one relation with block of statements. Each entry is a list of in-file locations that represents how much is the statement nested in COPY calls.

-   *Label storage*. The storage of sequence symbols that occur in the macro definition.

When macro is called, *macro invocation* object is created. It shares the content of a respective macro definition with an exception of calling parameters as they are assigned real value passed with the call. Also, it contains index to the top statement of the invocation.

The macro invocation is stored in the context’s *scope stack*.

### Scope stack

This stack (stack of `code_scope` objects) holds information about the scope of variable symbols. The scope changes when macro is visited. The initial scope is the open-code.

The stack elements contain:

-   In-scope variable symbols.

-   In-scope sequence symbols.

-   Pointer to the macro invocation (NULL if in open-code).

-   Branch counter (for ACTR instruction).

### COPY

HLASM context stores visited COPY members in the *copy strorage*.

COPY member definition is much more simple than the macro definition as it does not hold any more semantic information than the sequence of statements (the definition itself).

When copy is visited, copy member invocation is created and pushed in the copy stack of last entry of the *source stack*.

### Source stack and Processing stack

This stacks are responsible for the nests of opened files (source stack) and what they are opened for (processing stack). As the relation of source entry and processing entry is one-to-many, the information is stored in two arrays rather than one.

When [[statement processor|statement processors]] is changed (e.g. macro or copy definition is processed, lookahead is needed, ...), this information is stored in the processing stack. If a new file is opened during this change then source stack is updated as well.

Source stack contains:

-   *Source file identifier*

-   *Copy stack* – the nest of copy calls active for the source file.

-   *Processed statement location* – data that locates last processed statement in the source file.

Processing stack contains *processing kind*.

The reasoning of organizing this two stacks in such a way is:

1.  Context has enough information to fully reconstruct the statement.

2.  Easy retrieval of the correct copy stack for copy statement provider.

### ID storage

ID storage holds the string identifiers that are used by the open-code. It stores the string and retrieves a pointer. It is guaranteed that if two different strings with the same value are passed to the storage, the resulting pointers are equal.

It simplifies work with IDs and saves space.

### Variable symbols

In HLASM language, variable symbol is general term for symbols beginning with ampersand. However, they can be separated into several structures that capture a common behavior:

-   *SET symbols* – represent HLASM SET symbols.

-   *System variables* – represent HLASM system variables.

-   *Macro parameters* – represent HLASM macro parameters.

They inherit common abstract ancestor *variable symbol*. SET symbols are further divided into *SETA*, *SETB* and *SETC* symbols. Macro parameters are divided into *keyword* and *positional* parameters (see the picture below). They are stored in respective storage (global storage, scope stack, macro definition) that determines their scope.

<img src="img/variable_arch.svg" alt="The inheritance of variable symbols." />

### LSP context

The LSP context serves as the collection point for the data needed to answer the LSP requests. It is a part of the HLASM context to be able to pass on the LSP data between different parsed files.

The [[LSP data collector]] stores its values inside the LSP context tables.

### Ordinary assembly context

The above described structures aimed to describe the high-level part of the language (code generation). As we move closer to the resulting object code of the source file, the describing structures get complicated. Therefore, HLASM context contains object storing just this part of the processing.

<img src="img/ord_ctx_arch.svg" alt="The composition of ordinary assembly context" />

Ordinary assembly context consists of three main components (see the picture above):

1.  *Symbol storage*. Stores ordinary symbols.

2.  *Section storage*. Has notion of all generated sections, each section containing its location counters.

3.  *Symbol dependency tables*. Contains yet unresolved dependencies between symbols prior to the currently processed instruction.

#### Symbol

This class represents HLASM ordinary symbol. Besides its identifier and location, symbol contains *value* and *attributes* components.

Value  
can be assigned *absolute* or *relocatable* values. With addition to that, it can also be assigned an empty value stating that symbol is not yet defined.

Attributes  
structure holds symbol attributes like type, length, scale and integer.

#### Section

Section is a structure representing HLASM section (created by CSECT, DSECT, ...). It contains enumeration *section kind* describing type of the section prior to the used instruction. The structure also holds *location counter storage* with defined location counters.

#### Location counter

This structure contains data and operations for one location counter. The data is stored in helper sub-structure *location counter data*.

Location counter data  
is a structure defining current value of the location counter. It consists of:

-   *Storage* stating total number of bytes occupied by the location counter.

-   Vector of *spaces*, blocks of bytes with yet not known length.

-   Vector of *storage* between each space.

-   Currently valid *alignment* (used when data contain spaces).

The location counter value is transformable into a relocatable value. It is represented by structure *address*.

Address  
consists of:

-   Array of *bases*. A base is a beginning of a corresponding section. They serve as points of reference for the address.

-   Array of *spaces* that are present in the address.

-   *Offset* from the bases.

The common composition of an address is one base section (as the start of the address) and value of storage (as the offset from it).

The need for the whole array of bases to be present is because addresses from different sections can be arbitrarily added or subtracted. This information is needed as the correct sequence of arithmetic operations can reduce number of bases (even spaces) to zero and create absolute value. This value can be later used in places where a relocatable value would be forbidden.

Space  
is block of bytes with yet not known length. It is created in the active location counter when execution of counter’s operation can not be performed due to non previously defined ordinary symbols. See the different kind of spaces and the reason of creation in the table below.

| **Space Kind** |                                                **Creation**|
|:---------------|-----------------------------------------------------------:|
| Ordinary       |             when instruction outputs data of unknown length|
| LOCTR begin    |   when defining more than one location counter in a section|
| Alignment      |    when current alignment is unknown due to previous spaces|
| LOCTR set      |      when moving counter’s value to the address with spaces|
| LOCTR max      |  when moving counter’s value to the next available location|
| LOCTR unknown  |      when moving counter’s value to the yet unknown address|

When a space length becomes known, all addresses containing the spaces need to be updated (remove the space and append offset). Therefore, space structure contains an array of address listeners. Hence, when an address is assigned a relocatable value that contains the space, the address is added to its array. This serves as an easy point of space resolving.



ORG instruction can arbitrarily move location counter’s value forward and backward. With addition to that, ORG can also order location counter to set it’s value to the next available value (the lowest untouched address, see location counter in [[HLASM overview]]). Combining this with the possible spaces creation, location counter holds an array of the location counter data to properly set the next available value.

#### Symbol dependency tables

HLASM forbids cyclic symbol definition. This component maintains dependencies between symbols and detects possible cycles. Let us describe the main components of dependency resolving.

- **Dependant**  
is a structure used in the symbol dependency tables. It encapsulates objects that can be dependent on another. Dependant object can be a *symbol*, *symbol attribute* and *space*.

- **Dependable**  
interface is implemented by a class if its instance can contain dependencies. The interface has a method to retrieve a structure holding the respective *dependants*.

- **Resolvable**  
interface adds up to the dependable interface. It is implemented by objects that serve as values assignable to *depednants*. It provides methods to return *symbol value* with help of the dependency solver.

- **Dependency solver**  
is an interface that can return value of the symbol providing its identifier. It is implemented by Ordinary assembly context.

Having described building blocks, we can move to the symbol dependency tables composition.

- **Dependency map**  
is the primary storage of dependencies. It has *dependants* as keys and *resolvables* as values. The semantics for pair *(D,R)* is that D is dependent on the dependencies from R. Each time new dependency is added, this map is searched for cycle.

- **Dependency sources map**  
    serves as a source objects storage of a resolvable in the dependency map. Hence for the pair *(D,R)* from dependency map, source object of *R* is in the dependency source map under the key *D*.

    The source objects are statements. To be more specific, as one statement can be a source for more distinct resolvables, this source map only stores pointers to the *postponed statements storage*.

- **Postponed statements storage**  
holds statements that are sources of resolvables in dependency map. The reason they are stored is that they can not be checked yet as they contain dependencies. Therefore, they are postponed in the storage until all of the dependencies are resolved. Then they are passed to the respective checker.
