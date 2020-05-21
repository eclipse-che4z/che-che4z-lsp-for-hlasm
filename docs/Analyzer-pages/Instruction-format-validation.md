Instruction format validation
-----------------------------

One of the essential ways to provide results of the parsing to the user is through error messages. Many of these messages are created in *Instruction checker* which validates the usage of different kinds of instructions.

Instruction checker is an abstract class for various types of instructions. Its `check` method is being called from the [[instruction processors]] to check whether the specific instruction is used with correct parameters. As assembler and machine instructions have different formats, we derive separate *assembler* and *machine* checkers from the instruction checker. CA instructions do not have a derived checker class as they are all being checked during their interpretation.

The checkers need an access to the definitions of all possible instructions. These instructions are stored statically inside an object called *instruction*. It consists of 4 different containers:

-   *machine_instructions* is a map of instruction names to machine instruction object, which contains various data such as format, size or vector of instruction’s operands.

-   *mnemonic_codes* maps instruction names to their mnemonic code. The mnemonic codes are simplified versions of specific machine instructions, substituting one of the operands by a default value. The mnemonic code objects provides a list of operands to be substituted along with the original instruction name.

-   *assembler_instructions* is similar to the machine instructions. However, as the assembler instructions do not have formats, these classes only state minimum/maximum number of operands for specific instruction. In the **Assembler instruction checker** section below, we explain how the assembler instructions are validated.

-   *ca_instructions* only contains a list of possible CA instructions.

Both assembler and machine checker works in a similar manner:

1.  Either assembler or machine processor calls the `check` method of its respective checker. This method accepts the instruction name, the vector of used operands, the range of statement and the diagnostic collector.

2.  Checker finds the correct instruction based on the provided name and calls the `check` method of its instruction class, along with the same parameters as mentioned above.

3.  The instruction itself compares its possible operands with the used operands.

4.  More validations may be necessary, based on the instruction.

5.  In case of mismatch, a diagnostic is added to the passed diagnostic container.

### Machine instruction checker

All machine instructions have a precisely defined format which makes the validation based on these formats straightforward. Machine instructions checker operates with machine instructions and their mnemonic codes.

The formats are defined by several basic operands such as register or address and state which combination of these operands are acceptable. For example, instruction LR has format RR, which means it accepts only 2 arbitrary (but correct) registers.

<img src="img/org_diagram.svg" alt="Operand diagram for the ORG instruction." />

### Assembler instruction checker

Validation of assembler instructions is more complicated as there are no pre-defined formats for them. Each of them is described by custom operand diagrams, which demonstrate the dependencies and relations between operands of a specific instruction. An example of such diagram for the ORG instruction is shown in the picture above. As an addition to the basic operands used for machine instructions, each assembler instruction might have its own operands, called keywords.

Due to these irregularities, we derive instruction-specific classes from assembler instruction class. Each of them implements the `check` method, to provide the customized checking.

#### Data Definition checking

Data definition is a type of operand in HLASM. It represents data that is assembled directly into object code (see [[HLASM overview]]).

Since there are many types of data definition, there is a data definition subcomponent of instruction validation. Whenever any component of the project needs information about a data definition operand, it can use this subcomponent. It analyzes each type of data definition and is able to return its length, attributes and check its validity.

Each type is different and many have special conditions that must be met to be valid. That is why there is an abstract class `data_def_type_base`, which has 38 implementations — one for each type (including type extensions). The types are then available in a static associative map that maps names of types to their representations.
