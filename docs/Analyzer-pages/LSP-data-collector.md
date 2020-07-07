Data collection is necessary to be able to reply to LSP requests without the need to re-parse. During the parsing process, a component called the *LSP info processor* processes and stores this information. The main goal of this component is to collect as much information as possible to provide meaningful and complex replies to LSP requests while maintaining low memory and parse-time overheads.

The *LSP info processor* is invoked after each parsed and processed statement to collect and store the information it needs inside the *LSP context* (part of HLASM context).

### Supported LSP Language Features

The plugin implements four LSP language features:

- **hover**
The *hover* feature is invoked whenever a user moves his mouse cursor over a symbol for a short period of time. Typically a box with  information about the selected symbol appears right next to it.

- **complete**
The *complete* feature can be triggered by a custom set of events such as typing a specific character. The server responds with a list of possible correct options that can be inserted into the particular position.

- **go_to_definition**
The *go_to_definition* feature is invoked manually from the editor by selecting a symbol and consequently invoking the `go_to_definition` command. The editor “jumps” to the location of the currently selected symbol’s definition by moving the cursor to that location.

- **references**
The *references* feature is invoked in a similar manner to the *go_to_definition* feature. The results of the *references* feature are displayed as a list of all references to the selected symbol in the project, not just the definition of it.

### Supported HLASM Symbols

The symbols for which the user might call the above mentioned LSP features are *instruction symbols*, *variable symbols*, *sequence symbols* and *ordinary symbols*.

The *references* and the *go_to_definition* features are very similar for each symbol type and in most cases work as described above.

However, there are two exceptions to the standard behavior of the *go_to_definition* feature. First, the command jumps to the definition of an instruction symbol only for macros (to the macro definition file). For the built-in instructions, the feature simply jumps to the first occurrence of the instruction. Second, the command used on an ordinary COPY symbol jumps to the corresponding copy file.

On the other hand, the responses to the *hover* and the *complete* features vary for each symbol type and are described in the following tables:

| **Symbol Type** | **Hover Contents**                                        |
|:----------------|:----------------------------------------------------------|
|instruction| the type of the instruction, the syntax of its parameters the version (macros only), the documentation|
| variable        | the type of the variable — bool/string/number             |
| sequence        | the position of the definition                            |
| ordinary        | absolute/relocatable, the value, the values of attributes |
| (COPY)          | the name of the copy file                                 |

<br />

| **Symbol Type** | **Trigger Characters, Events**        | **Response**                |
|:----------------|:--------------------------------------|:----------------------------|
|instruction| A-Z,@,$,\# after any number of spaces from the start of the line | built-in HLASM instructions + already used macros|
|variable|&                                       | variable symbols defined before the current line in the current scope|
|sequence|.                                       | sequence symbols defined before the current line   |
| ordinary        | *not implemented*                     | *not implemented*           |
