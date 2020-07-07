HLASM differentiates two kinds of expressions: *Conditional Assembly* (CA) and *Machine* expressions. CA expressions appear in conditional assembly, which is processed during compilation. Machine expressions are used with assembler and machine instructions.

# CA Expressions

HLASM evaluates CA expressions during assembly generation. For further details, refer to the [[HLASM overview]].

We employ the ANTLR 4 Parse-Tree Visitors during the expression evaluation. For further detail on ANTLR, refer to [[Third party libraries]]

The HLASM CA expression is conceptually similar to expressions in other languages: they support unary and binary operators, functions, variables and literals. In HLASM, each expression has a type. *Arithmetic*, *Logic*, *Character* expressions are supported. We implement the logic in the following classes:

`expression`  
A pure virtual class that defines a shared interface, operators, and functions. The class also implements evaluation logic for terms and factors.

`diagnostic_op`  
The concept of *diagnostics* is fundamental. During the evaluation of an expression, an error can occur (syntactic or semantic). Hence, we try to improve the user experience by reporting diagnostics. Each instance of `expression` has a pointer to `diagnostic_op` associated to it. If the pointer is `null`, it is considered error-free. During the evaluation of a child expression, the parent checks for errors and propagates the error upwards. The checks and propagation are implemented by the `copy_return_on_error` macro, which must be called immediately before the creation of a new expression during evaluation.

The `expression` class implements the evaluation as follows: A `std::deque` of `expression` pointers is passed. The evaluation iterates the list from left to right. Functions, binary, and unary operators consume the rest of the deque.

Some expression symbols can be either HLASM keywords or variable identifiers (see the example below). Therefore, the resolution of symbols is complicated and cannot be done straight, but instead during the evaluation time. The order of the expression’s terms and the previous evaluation context is crucial for disambiguation.

<!-- -->
        name        operation   operands
    	
        AND         EQU         1
        NOT         EQU         0
                    AIF         (NOT AND AND AND).LAB   <- EVALUATES TO (!1 & 1)

- `keyword_expression`  
Helper class that represents HLASM keywords in expressions. It determines a keyword type from a string, containing its arity (unary, binary) and priority.

- `logic_expression`  
Represents a boolean expression.

- `arithmetic_expression`  
Represents an arithmetic expression.

- `arithmetic_logic_expr_wrapper`  
HLASM language supports expressions with operands of mixed types. For more straightforward and readable use of arithmetic and logical expressions, this class wraps them under one class.

- `character_expression`  
Represents a character expression.

- `ebcdic_encoding`  
This class defines a custom EBCDIC literal and provides helper functions for conversion between EBCDIC and ASCII. EBCDIC is a character encoding used on IBM mainframes. It has a different layout to ASCII.

<img src="img/ebcdic.png" alt="EBCDIC layout. Taken from https://i.stack.imgur.com/h3u5A.png."/>

- `error_messages`  
A static class with a list of all `diagnostic_op`s that can be generated from expressions.

## CA expression evaluation

In the previous section, we described the representation of the CA expressions themselves. In this section, we explain the coupling of CA expressions with grammar.

The `expression_evaluator` encapsulates the coupling logic between the grammar and the expression logic. That is, the evaluator has a notion about grammar, which translates into C++ expression logic.

The top-level expression first gathers a list of space-separated expressions. The evaluation must be done using a list from left to right (not using a tree) as any token may be a keyword (such as the operator `AND`) or variable identifier, depending on the position in an expression (using language keywords as identifiers is allowed in HLASM). `expression::evaluate` provides the disambiguation.

During its work, the evaluator substitutes variable and ordinary symbols for their values. To know which values to substitute, the evaluator is given *evaluation context*. This consists of objects that are required for correct evaluation: *HLASM context* for symbol values, *attribute provider* for values of symbol attributes that are not yet defined and *library provider* for evaluation of some types of symbol attributes.

Lookahead is triggered in conditional assembly expressions when evaluation visits a yet undefined ordinary symbol. As this might be a rather demanding operation, the expression evaluator uses *expression analyzer*. It looks for all the undefined symbol references in the expression and collects them in a common collection. Then, the lookahead is triggered to look for all references in the collection. Hence, it is triggered once per expression rather than any time an undefined symbol reference is found.

# Machine expressions

In HLASM, machine expressions are used as operands of machine and assembler instructions. Their result is a simple absolute number or an address.

We use a standard infix tree representation of expressions. There is an interface, `machine_expression`, which is implemented by several classes that represent operators and terms. Each binary operator holds two expressions — the left and right operands. Terms are leaf classes that do not hold any other expressions and directly represent a value. There are several classes representing different terms valid in machine expressions:

-   `mach_expr_constant` represents a number.

-   `mach_expr_symbol` represents an ordinary symbol.

-   `mach_expr_data_attr` represents an attribute of a symbol (e.g. `L’SYM` is length of symbol `SYM`)

-   `mach_expr_location_counter` represents a location counter represented by an asterisk in expressions.

-   `mach_expr_self_def` represents a self defining term (e.g. `X’1F’`)

The following example shows a representation for one specific expression.

<img src="img/mach_expr_example.svg" alt="Example representation of the machine expression (A-4)+L’B." />

Machine expressions can also evaluate the expressions they represent. The evaluation is done in a recursive manner. It is fairly simple when there are no symbols used in the expression — each node in the tree computes the result with basic arithmetic operations.

However, the process can get tricky since expressions might contain e.g. `mach_expr_symbol`, whose value is dependant on symbols defined in other parts of source code. Moreover, the result of a machine expression can be an absolute value (a number) or relocatable value (an address). The process of symbol resolution is explained in the *symbol dependency tables* section of [[HLASM context tables]].