Instruction Format Validation
-----------------------------

One of the essential ways to provide results of parsing to the user is through error messages. Many of these messages are created in the *Instruction checker* which validates the usage of different kinds of instructions.

Instruction checker is an abstract class for various types of instructions. Its `check` method is called from the [[instruction processors]] to check whether the specific instruction is used with correct parameters. As assembler and machine instructions have different formats, we derive separate *assembler* and *machine* checkers from the instruction checker. CA instructions do not have a derived checker class as they are all checked during their interpretation.

The checkers need access to the definitions of all possible instructions. These instructions are stored statically inside an object called an *instruction*. It consists of 4 different containers:

-   *machine_instructions* is a map of instruction names to machine instruction objects, which contains various data such as format, size and the vector of the instruction’s operands.

-   *mnemonic_codes* maps instruction names to their mnemonic code. The mnemonic codes are simplified versions of specific machine instructions, substituting one of the operands with a default value. The mnemonic code objects provides a list of operands to be substituted along with the original instruction name.

-   *assembler_instructions* is similar to the machine instructions. However, as the assembler instructions do not have formats, these classes only state the minimum/maximum number of operands for a specific instruction. In the **Assembler Instruction Checker** section below, we explain how the assembler instructions are validated.

-   *ca_instructions* only contains a list of possible CA instructions.

Both the assembler and machine checkers work in a similar manner:

1.  Either the assembler or machine processor calls the `check` method of its respective checker. This method accepts the instruction name, the vector of the operands used, the range of the statement and the diagnostic collector.

2.  The checker finds the correct instruction based on the provided name and calls the `check` method of its instruction class, along with the same parameters as mentioned above.

3.  The instruction itself compares its possible operands with the used operands.

4.  More validations might be necessary, based on the instruction.

5.  If there is a mismatch, a diagnostic is added to the passed diagnostic container.

### Machine Instruction Checker

All machine instructions have a precisely defined format which makes validation based on these formats straightforward. The machine instructions checker operates with machine instructions and their mnemonic codes.

The formats are defined by several basic operands such as register and address, and state which combination of these operands are acceptable. For example, the instruction LR has the format RR, which means it accepts only 2 arbitrary (but correct) registers.

<img src="img/org_diagram.svg" alt="Operand diagram for the ORG instruction." />

### Assembler Instruction Checker

Validation of assembler instructions is more complicated as there are no pre-defined formats for them. Each of them is described by custom operand diagrams, which demonstrate the dependencies and relations between operands of a specific instruction. An example of such a diagram for the ORG instruction is shown in the picture above. As an addition to the basic operands used for machine instructions, each assembler instruction might have its own operands, called keywords.

Due to these irregularities, we derive instruction-specific classes from the assembler instruction class. Each of them implements the `check` method, to provide the customized checking.

#### Data Definition Checking

A data definition is a type of operand in HLASM. It represents data that is assembled directly into object code (see [[HLASM overview]]).

Since there are many types of data definition, there is a data definition subcomponent of instruction validation. Whenever any component of the project needs information about a data definition operand, it can use this subcomponent. It analyzes each type of data definition and is able to return its length and attributes and check its validity.

Each type is different and many have special conditions that must be met to be valid. Hence, there is an abstract class `data_def_type_base`, which has 38 implementations — one for each type (including type extensions). The types are then available in a static associative map that maps names of types to their representations.