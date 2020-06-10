Due to the macro definitions, copy file includes and statement generation, it is difficult to determine which statement to process next. For this reason, we define abstraction over various sources of statements called *statement providers*.

Unlike statement processors, statement providers are ordered based on the priority (lower index, greater priority):

1.  Macro definition statement provider
2.  Copy definition statement provider
3.  Opencode statement provider

In each iteration of the [[processing manager]], providers are asked whether they have statements to provide based on the ordering. That is because after each iteration, a provider with greater priority than the previously used one can be activated.

| **Processors** | Macro provider ends | Copy provider ends | Opencode provider ends |
|:---------------|:---:|:---:|:---:|
| **Opencode**   | continue | continue | finish |
| **Copy**       | finish | continue | finish |
| **Macro**      | finish | continue | finish |
| **Lookahead**  | finish | continue | finish |

For the main loop to be correctly defined, the end of the opencode provider triggers the terminal condition for all statement processors. Hence, when the opencode provider finishes, all the processors finish as well and the processing ends (see the table above).

#### Statement Passing

In HLASM language, it is difficult to parse statements into one common structure due to its *representational ambiguity*; the major difference between operand fields of different instruction formats. Moreover, when parsing statements, the instruction format can be yet unknown. Therefore, operand fields are stored as strings. This means that during statement passing when instruction format is deduced, the provider has the responsibility to produce the correct statement format. The following steps are applied in the statement passing (see also the picture below):

1.  The provider retrieves the instruction field part of the statement.

2.  The provider calls the processor method `get_processing_status` with the instruction field as a parameter.

3.  The return value of the call determines the required format of the operand field for the processor; the whole statement can be retrieved correctly.

4.  The provider returns the correctly formatted statement to the processor.

<img src="img/process_next.svg" alt="Process of statement passing."/>

#### Copy and Macro Definition Provider

These providers are activated when a COPY instruction copies a file into the source code or when a macro is visited, respectively. They provide a sequence of statements to an arbitrary processor until all statements from the copy or macro definition are provided. After that, if there is no nested invocation, a provider with a lower priority is selected.

To avoid infinite macro recursion, the HLASM language itself has a restriction for the level of nested macro invocation depending on the complexity of nested macros. We set the limit to 100 as it suffices in all tested code.

For COPY members, recursion is forbidden.

#### Opencode Provider

The opencode provider is active as long as there are statements in the source file. It retrieves statements from the source code with the help of the [[lexer]] and [[parser]].

### Statement Field Parser

The statement field parser is an interface passed to the statement providers by the processing manager. It is implemented by the [[parser]].

At first, it is used during statement passing. In some cases the provider is requested to provide a specific format of a string-stored statement. The string is re-parsed with the according format. Then the field is returned back to the statement provider.

Another use of the field parser is in the opencode processor as model statements are resolved there. After variable symbol substitution, the resulting string field is re-parsed with the field parser.
