Due to the macro definitions, copy file includes and statement generation, it is difficult to state which statement should be processed next. For this reason, we define abstraction over various sources of statements called *statement providers*.

In contrary to statement processors, statement providers are ordered based on the priority (lower index, greater priority):

1.  Macro definition statement provider
2.  Copy definition statement provider
3.  Opencode statement provider

In each iteration of [[processing manager]], providers are asked whether they have statements to provide based on the ordering. That is because after each iteration, a provider with greater priority than the previously used one can be activated.

| **Processors** | Macro provider ends | Copy provider ends | Opencode provider ends |
|:---------------|:---:|:---:|:---:|
| **Opencode**   | continue | continue | finish |
| **Copy**       | finish | continue | finish |
| **Macro**      | finish | continue | finish |
| **Lookahead**  | finish | continue | finish |

For the main loop to be correctly defined, the end of opencode provider triggers terminal condition for all statement processors. Hence, when opencode provider finishes then all the processors finish as well and the processing ends (see the table above).

#### Statement passing

In HLASM language, it is difficult to parse statements into one common structure due to its *representational ambiguity*; the major difference between operand fields of different instruction formats. Moreover, when parsing statements, the instruction format can be yet unknown. Therefore, operand fields are stored as strings. This means that during statement passing when instruction format is deduced, the provider has responsibility to produce correct statement format. The following steps are applied in the statement passing (see also the picture below):

1.  Provider retrieves the instruction field part of the statement.

2.  Provider calls processor method `get_processing_status` with instruction field as a parameter.

3.  Return value of the call determines the required format of the operand field for the processor; the whole statemement can be retrieved correctly.

4.  Provider returns statement with correct format to the processor.

<img src="img/process_next.svg" alt="The process of statement passing."/>

#### Copy and Macro definition Provider

These providers are activated when COPY instruction copies a file into the source code or when a macro is visited, respectively. They provide a sequence of statements to an arbitrary processor until all statements from the copy or macro definition are provided. After that, if there is no nested invocation, a provider with lower priority is selected.

To avoid infinite macro recursion, HLASM language itself has a restriction for the level of nested macro invocation depending on the complexity of nested macros. We set the limit to 100 as it suffices in all tested code.

For COPY members, recursion is forbidden.

#### Opencode Provider

Opencode provider is active as long as there are statements in the source file. It retrieves statements from the source code with help of [[lexer]] and [[parser]].

### Statement field parser

Statement field parser is an interface passed to the statement providers by processing manager. It is implemented by the [[parser]].

At first, it is used during statement passing. In some cases provider is requested a specific format of a string-stored statement. The string is re-parsed with the according format. Then the field is returned back to the statement provider.

Another use of field parser is in opencode processor as model statements are resolved there. After variable symbol substitution, the resulting string field is re-parsed with field parser.
