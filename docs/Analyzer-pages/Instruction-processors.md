Opencode processor divides processing of HLASM instruction types into several *instruction processors*. Each processor is responsible for processing instructions that belong to one instruction type.

As a format of some instruction kinds can be rather complicated, instruction processors contain *[[Instruction format validators|Instruction format validation]]*. They check the statement to validate the correctness of used operand format as well as the correctness of the actual operand values.

During the instruction processing, processors work with HLASM *[[expressions]]*. They need to be evaluated to correctly perform the processing.

There are four specialized instruction processors:

- **Macro IP**  
looks up for macro definition in HLASM context tables and calls it.

- **Assembler and Machine IP**  
processes assembler and machine instructions to retain consistency in HLASM context tables.

- **Conditional assembly IP**  
executes conditional assembly instructions.

See the current list of processed instruction in the following table.

| **IP**                   |                         **Processed instructions**|
|:-------------------------|--------------------------------------------------:|
| **Assembler**            |  \*SECT, COM, LOCTR, EQU, DC, DS, COPY, EXTRN, ORG|
| **Machine**              |               *Instruction format validation only*|
| **Macro**                |                                              *ANY*|
| **Conditional Assembly** |    SET\*, GBL\*, ANOP, ACTR, AGO, AIF, MACRO, MEND|
