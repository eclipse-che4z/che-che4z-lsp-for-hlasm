The opencode processor divides processing of HLASM instruction types into several *instruction processors*. Each processor is responsible for processing instructions that belong to one instruction type.

As the format of some instruction types can be rather complicated, instruction processors contain *[[Instruction format validation|instruction format validators]]*. They check the statement to validate the correctness of the used operand format as well as the correctness of the actual operand values.

During instruction processing, processors work with HLASM *[[expressions]]*. They need to be evaluated to correctly perform the processing.

There are four specialized instruction processors:

| **IP**                   |                         **Processed instructions**|
|:-------------------------|--------------------------------------------------:|
| **Assembler**            |  \*SECT, COM, LOCTR, EQU, DC, DS, COPY, EXTRN, ORG|
| **Machine**              |               *Instruction format validation only*|
| **Macro**                |                                              *ANY*|
| **Conditional Assembly** |    SET\*, GBL\*, ANOP, ACTR, AGO, AIF, MACRO, MEND|

- **Macro IP**  
Looks for a macro definition in HLASM context tables and calls it.

- **Assembler and Machine IP**  
Processes assembler and machine instructions to retain consistency in HLASM context tables.

- **Conditional Assembly IP**  
Executes conditional assembly instructions.