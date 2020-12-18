HLASM differentiates two kinds of expressions: *Conditional Assembly* (CA) and *Machine* expressions. CA expressions appear in conditional assembly, which is processed during compilation. Machine expressions are used with assembler and machine instructions.

# CA Expressions

HLASM evaluates CA expressions during assembly generation. For further details, refer to the [[HLASM overview]].

The HLASM CA expression is conceptually similar to expressions in other languages: they support unary and binary operators, functions, variables and literals. In HLASM, each expression has a type. *Arithmetic*, *Logic*, *Character* expressions are supported. We implement the logic in the following classes:

- `ca_expression`  
A pure virtual class that defines a shared interface, operators, and functions.

- `ca_unary_operator` and `ca_binary_operator`  
These virtual classes provide a point of inheritance for specialized classes that represent binary and unary operators that are found in HLASM. There are basic arithmetic operators (e.g. plus, minus) or function operators (e.g. `NOT`, `SLL`).

- Term classes `ca_function`, `ca_constant`, `ca_string`, `ca_symbol`, `ca_symbol_attribute`, `ca_var_sym`  
These classes all inherit from the `ca_expression` class. Each of them represents a term that can be used in HLASM conditional assembly expressions.   
The following examples show the usage of each class:

    | Class                   | Examples of terms they represent  |
    |:------------------------|:----------------------------------|
    | `ca_function`           | `FIND('abc','d')`, `DCLEN('abcd')`|
    | `ca_constant`           | `42`, `C'A'`                      |
    | `ca_string`             | `'abc'`, `'**findme**'(3,*)`      |
    | `ca_symbol`             | `R1`                              |
    | `ca_symbol_attribute`   | `L'DC_HALF`, `T'&VAR`             |
    | `ca_var_sym`            | `&VAR`, `&VAR(1,2,3)`             |

- `ca_expr_list`  
This is the class that holds a list of instantiated objects of the above stated classes.   
In logical expressions, some symbols can be either expression operators or ordinary symbol identifiers (see the example below). Therefore, the resolution of symbols can be complicated and cannot be done straight during parsing. This class holds the list of the terms that contributed to the logical expressions and contains an algorithm that disambiguates the expression (from the example logical expression `(NOT AND AND AND)`, the object of this class would hold four `ca_symbol` objects, one `NOT` and three `AND`s.

<!-- -->
        AND    EQU   1
               AIF   (NOT AND AND AND).LAB   <- EVALUATES TO (!1 & 1)

- `ca_expr_policies`  
Static classes that provide useful information about each built-in function (e.g. return type, number of parameters) and operators. The classes are divided into arithmetic, logical and character because some operators have different meanings in different types of expressions (like logical and arithmetic `AND`).

- `ebcdic_encoding`  
This class defines a custom EBCDIC literal and provides helper functions for conversion between EBCDIC and ASCII. EBCDIC is a character encoding used on IBM mainframes. It has a different layout to ASCII.  

<img src="img/ebcdic.png" alt="EBCDIC layout. Taken from https://i.stack.imgur.com/h3u5A.png."/>

## Resolution and evaluation of CA expressions

To evaluate a CA expression, the expression object has to be resolved once. Each class overrides the `resolve_expression` method which typically checks whether it has the correct number of fields and that the fields are of the correct type. The `ca_expr_list` class does the most of the resolving work.  
It contains an algorithm that creates an expression tree from its list of expression terms. This tree is then used for further evaluation.

When an expression was resolved once, it can be properly evaluated.  
During the evaluation, variable and ordinary symbols are substituted for their values. To determine which values to substitute, the `evaluate` method is given *evaluation context*. This consists of objects that are required for correct evaluation: *HLASM context* for symbol values, *attribute provider* for values of symbol attributes that are not yet defined and *library provider* for evaluation of some types of symbol attributes.

Lookahead is triggered in conditional assembly expressions when an evaluation visits a yet undefined ordinary symbol. As this might be a rather demanding operation, the `ca_expression` class contains the method `get_undefined_attributed_symbols`. It looks for all the undefined symbol references in the expression and collects them in a common collection. Then, the lookahead can be triggered to look for all references in the collection. Hence, it is triggered once per expression rather than any time an undefined symbol reference is found.

# Machine expressions

In HLASM, machine expressions are used as operands of machine and assembler instructions. Their result is a simple absolute number or an address.

We use a standard infix tree representation of expressions. There is an interface, `machine_expression`, which is implemented by several classes that represent operators and terms. Each binary operator holds two expressions — the left and right operands. Terms are leaf classes that do not hold any other expressions and directly represent a value. There are several classes representing different terms valid in machine expressions:

-   `mach_expr_constant` represents a number.

-   `mach_expr_symbol` represents an ordinary symbol.

-   `mach_expr_data_attr` represents an attribute of a symbol (e.g. `L'SYM` is length of symbol `SYM`)

-   `mach_expr_location_counter` represents a location counter represented by an asterisk in expressions.

-   `mach_expr_self_def` represents a self defining term (e.g. `X'1F'`)

The following example shows a representation for one specific expression.

<img src="img/mach_expr_example.svg" alt="Example representation of the machine expression (A-4)+L'B." />

Machine expressions can also evaluate the expressions they represent. The evaluation is done in a recursive manner. It is fairly simple when there are no symbols used in the expression — each node in the tree computes the result with basic arithmetic operations.

However, the process can get tricky since expressions might contain e.g. `mach_expr_symbol`, whose value is dependant on symbols defined in other parts of source code. Moreover, the result of a machine expression can be an absolute value (a number) or relocatable value (an address). The process of symbol resolution is explained in the *symbol dependency tables* section of [[HLASM context tables]].
