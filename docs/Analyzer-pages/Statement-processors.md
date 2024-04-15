The motivation for distinguishing different statement processors is the complexity of HLASM language. There are many cases when the same statements require different processing under different circumstances (e.g. a COPY instruction in a macro is handled differently to in opencode, or lookahead mode can accept statements that would fail when processed by ordinary processing).

During processing, statement processing kinds can be nested. Hence, statement processors are dynamically assigned to the manager when needed and removed from it when they finish. This happens when the processor encounters a specific statement (e.g. a statement with a special instruction or previously undefined sequence symbol; see the table at the end of this page for further information). For this purpose, they use the *processing state listener* interface (implemented by the processing manager), which tells the manager to change the current processor.

#### Statement Structure

A statement consists of *statement fields* — *label field*, *instruction field*, *operands field*, and *remark field*. It is used by statement processors and produced by statement providers.

The abstract class *HLASM statement* is the ancestor for all statement related classes. Then, there are the abstract classes *deferred statement* and *resolved statement*. Deferred statement has its operand field stored in unresolved — deferred — format (in code stored as string). This statement is created when the actual instruction is not yet known before the statement is created (see the example below). Resolved statements are complementary to the deferred statements as their instruction and operand format are known before the statement is created.

    *VALUE OF INSTRUCTION IN DEFERRED STATEMENT IS PARAMETER OF MACRO MAC
         MACRO
         MAC      &INSTR
         &INSTR   3(2,0),13      <- deferred statement
         MEND


#### Copy and Macro Definition Processors

Both of these statement processors handle statement collecting, forming definition structure and storing it into HLASM context tables. They come into effect when a COPY instruction or macro definition is encountered in the source code.

The statements collected inside copy or macro definitions are mainly deferred statements. That is because variable symbols can not be resolved inside the definition, and because HLASM allows instruction aliasing (renaming instructions). Therefore, during the processing of a definition, as the instruction field is parsed, the format of its operands is unknown. It is fully deduced when the definition is handed over to the provider and processed by the opencode processor.

However, some statements in the macro and copy definitions forbid aliasing and the operand format can be deduced immediately (e.g. conditional assembly instructions in macro definition). This leads to the processors necessity to ask the provider to retrieve the statement with the correct format – which is determined based on the instruction being provided.

#### Lookahead Processor

The lookahead processor is activated when a currently processed conditional assembly statement requires a value of an undefined ordinary or sequence symbol. It looks through the succeeding statements and finishes when the target symbol is found or when all statement providers finish. Then the processing continues from where the lookahead started.

#### Opencode Processor

The functionality of the opencode processor (`ordinary_processor` class) can be described as follows:

1.  If a model statement is encountered, it substitutes the variable symbols and resolves the statement.

2.  It checks the statement for validity.

3.  It performs instruction by updating HLASM context tables with the help of *instruction processors*.

4.  It is passed *processing tracer* by the manager. Each time a statement is processed by the opencode processor, it triggers the processing tracer. The tracer serves as a listener pattern used by the *[[macro tracer]]*.

In the table below, we can see that there is no field that starts the opencode processor. That is because this processor is set as a default by the manager. Additionally, the copy processor does not finish itself during its work as it can only be finished by its *terminal condition*.

A terminal condition can be triggered by a finishing provider. It indicates that the processor needs to finish its work when a specific provider exhausted its statement stream.

| **Processor** | END instruction  | COPY instruction    | MACRO instruction    | MEND instruction    | undefined symbol |
|:--------------|:---:|:---:|:---:|:---:|:---:|
| **Opencode**  | finish | start Copy | start Macro | continue | start Lookahead |
| **Copy**      | continue | continue | continue | continue | continue |
| **Macro**     | continue | start Copy | continue | finish | continue |
| **Lookahead** | finish | start Copy | continue | continue | continue |
