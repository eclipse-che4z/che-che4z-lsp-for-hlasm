The lexer's responsibility is to read a source string and break it into tokens — small pieces of text with special meanings. The most important properties of the lexer are:

-   each token has a location in the source text,

-   has the ability to check whether all characters are valid in the HLASM source,

-   can jump backward and forward in the source file if necessary (for implementation of instructions like AGO and AIF). Because of this, it is not possible to use a standard lexing tool.

The latter point necessitated designing a custom lexer for HLASM. We had a number of reasons to do so. The HLASM language is complex. It was first introduced several decades ago and, during this long time, the language was subjected to prolonged development which has resulted in complexity. Also, it contains some features, for example, `AREAD` or `COPY`, that can alter the source code at parse time.

Conventional lexing tools are most often based on regular expressions. As discussed above, there are several difficulties that one must consider while designing a lexer for this particular language. A regular expression-based lexer would be too difficult or even impossible to design.

One could match separate characters from the input and let the parser or semantic analysis deal with some of the described problems. This drastic solution would cost in performance, as parsers are usually more performance demanding.

### Source File Encodings

Source code encodings differ for the used libraries. All strings are encoded in `UTF` as follows:

- `UTF-8` LSP string encoding,

- `UTF-16` offsets (positions in source code) in LSP,

- `UTF-32` ANTLR 4 source code representation.

### Lexer Components

<img src="img/lexer_arch.svg" alt="Lexer architecture overview." />

 Note that there are two `input_source`s and there are many `token`s generated. The *AINSERT buffer* has higher priority. Specifically, if the buffer is non-empty, the lexer consumes the input from this buffer.
 
 Besides the custom lexer, we altered ANTLR’s classes `Token`, `TokenFactory` and `ANTLRInputStream` (see [[Third party libraries#ANTLR 4 Pipeline|ANTLR 4 pipeline]]). The reason was to add custom attributes to tokens that are vital for later stages of the HLASM code analysis (parsing, semantic analysis etc.). Lexer functionality is implemented in the following classes (see the picture above):

- `token`
    Implements ANTLR’s class `Token` and extends it by adding properties important for location of the token within the input stream. As the LSP protocol works with offsets encoded in `UTF-16` and ANTLR 4 works with `UTF-32` encoding, we add attributes for `UTF-16` positions too.

    A token does not carry the actual text from the source but instead references the position in code (unlike `CommonToken`). Note that the position of a token is vital for further analysis.

    |                |                                                |
    |:---------------|-----------------------------------------------:|
    | **IGNORED**    |    sequence of characters ignored in processing|
    | **COMMENT**    |                           commentary statements|
    | **SPACE**      |                            a sequence of spaces|
    | **IDENTIFIER** |                               symbol identifier|
    | **ORDSYMBOL**  |                      Ordinary symbol identifier|
    | **PROCESS**    |                         process statement token|
    | **NUM**        |                                          number|
    | **ATTR**       |apostrophe that serves as an attribute reference|
    | **ASTERISK, SLASH, MINUS, PLUS, LT, GT, EQUALS, LPAR, RPAR** |                              expression tokens|
    | **DOT, COMMA, APOSTROPHE, AMPERSAND, VERTICAL**               |                         special meaning tokens|

    A notable component of HLASM language complexity is the absence of a *string* token (see the table above). The lexer does not generate this token due to the existence of model statements. There, a variable symbol can be written anywhere in the statement (even in the middle of the string), which significantly restricts the lexer.

- `token_factory`  
    Produces tokens of the previously described custom type `token`.

- `input_source`  
    Implements [`ANTLRInputStream`](https://www.antlr.org/api/Java/org/antlr/v4/runtime/ANTLRInputStream.html) which encapsulates source code. This implementation adds an API for resetting, rewinding and rewriting input.

    Note the usage of `UTF` encodings: `_data` (source code string) and positions/indices in API are in `UTF-32`; `getText` returns a `UTF-8` string.

- `lexer`  
   Based on ANTLR’s [`TokenSource`](https://www.antlr.org/api/Java/org/antlr/v4/runtime/TokenSource.html) class. As with most lexers, it is also, in principle, a finite state machine. The most important difference compared to conventional FSMs and other lexers is the added communication interface that connects the parser and the instruction interpreter with the lexer. Other unconventional features include input rewinding (to support instructions such as `AREAD`), lexing from parallel sources (`AINSERT` buffer) and a helper API for subsequent processing stages.

Important functions:

- `nextToken()`  
Implements the main functionality: lexing and emitting tokens. Before lexing, the function uses the right input stream (either the source code or `AINSERT` buffer if not empty). After choosing the right input source, the lexer emits a token. HLASM introduces a *continuation* symbol (an arbitrary non-blank symbol in the continuation column) that breaks one logical line into two or more source-code lines.

- `create_token()`  
Creates a token of a given type. The lexer’s internal state gives the position of the token.

- `consume()`  
Consumes a character from the input stream and updates the lexer’s internal state (used in `create_token()`).

- `lex_tokens()`  
Lexes of most of the token types.

- `lex_begin()`  
Up to a certain column, the input can be ignored (can be set in HLASM).

- `lex_end()`  
Lexes everything after a continuation symbol.
