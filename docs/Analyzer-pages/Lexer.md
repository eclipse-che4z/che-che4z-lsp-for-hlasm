Lexer’s responsibility is to read source string and break it into tokens — small pieces of text with special meaning. The most important properties of the lexer:

-   each token has a location in the source text,

-   has the ability to check whether all characters are valid in the HLASM source,

-   can jump in the source file backward and forward if necessary (for implementation of instructions like AGO and AIF). Because of this, it is not possible to use any standard lexing tool, and the lexer has to be implemented from scratch.

As previously mentioned, we designed a custom lexer for HLASM. We had a number of reasons to do so. HLASM language is complex. It was first introduced several decades ago and, during this long time, the language was subjected to development. Such a long time period has made the HLASM language complex. Also, it contains some aggressive features, for example, `AREAD` or `COPY`, that can alter the source code at parse time.

Conventional lexing tools are most often based on regular expressions. As discussed above, there are several difficulties that one must consider while designing lexer for this particular language. A regular expression-based lexer would be too difficult or even impossible to design.

One could match separate characters from the input and let the parser or semantic analysis deal with some of the described problems. This drastic solution would cost performance, as parsers are usually more performance demanding.

### Source file encodings

Source code encodings differ for the used libraries. All strings are encoded in `UTF` as follows:

- `UTF-8` LSP string encoding,

- `UTF-16` offsets (positions in source code) in LSP,

- `UTF-32` ANTLR 4 source code representation.

### Lexer components

<img src="img/lexer_arch.svg" alt="Lexer architecture overview." />

 Note, there are two `input_source`s and there are many `token`s generated. The *AINSERT buffer* has higher priority. Specifically, if the buffer is non-empty, lexer consumes the input from this buffer.
 
 Beside of the custom lexer, we altered ANTLR’s classes `Token`, `TokenFactory` and `ANTLRInputStream` (see Using antlr in [[Third party libraries]]). The reason was to add custom attributes to token that are vital for later stages of the HLASM code analysis (parsing, semantic analyses, etc.). Lexer functionality is implemented in following classes (see the picture above):

- `token`
    implements ANTLR’s class `Token` and extends it by adding properties important for location of the token within the input stream. As the LSP protocol works with offsets encoded in `UTF-16` and ANTLR 4 works with `UTF-32` encoding, we add attributes for `UTF-16` positions too.

    Token does not carry the actual text from the source but instead references the position in code (unlike `CommonToken`). Note that the position of a token is vital for further analysis.

    |                |                                               |
    |:---------------|----------------------------------------------:|
    | **IGNORED**    |   sequence of characters ignored in processing|
    | **COMMENT**    |                          commentary statements|
    | **EOLLN**      |          token signalling the end of statement|
    | **SPACE**      |                           a sequence of spaces|
    | **IDENTIFIER** |                              symbol identifier|
    | **ORDSYMBOL**  |                     Ordinary symbol identifier|
    | **PROCESS**    |                        process statement token|
    | **NUM**        |                                         number|
    | **ATTR**       |  apostrophe that serves as attribute reference|
    | **ASTERISK, SLASH, MINUS, PLUS, LT, GT, EQUALS, LPAR, RPAR** |                              expression tokens|
    | **DOT, COMMA, APOSTROPHE, AMPERSAND, VERTICAL**               |                         special meaning tokens|

    Interesting remark of HLASM language complexity is absence of *string* token (see the table above). Lexer does not generate this token due to the existence of model statements. There, variable symbol can be written anywhere in the statement (even in the middle of the string), what significantly restricts lexer.

- `token_factory`  
produces tokens of previously described custom type `token`.

- `input_source`  
    implements [`ANTLRInputStream`](https://www.antlr.org/api/Java/org/antlr/v4/runtime/ANTLRInputStream.html) which encapsulates source code. This implementation adds API for resetting, rewinding and rewriting input.

    Note the usage of `UTF` encodings: `_data` (source code string) and positions/indices in API are in `UTF-32`; `getText` returns `UTF-8` string.

- `lexer`  
is based on ANTLR’s [`TokenSource`](https://www.antlr.org/api/Java/org/antlr/v4/runtime/TokenSource.html) class. As most lexers, it is also, in principle, a finite state machine. The most important difference compared to conventional FSMs and other lexers is added communication interface that connects the parser and the instruction interpreter with the lexer. Unusual is also input rewinding (to support `AREAD`, for example), lexing from parallel sources (`AINSERT` buffer) and some helper API for subsequent processing stages.

Important functions:

- `nextToken()`  
implements main functionality: lexes and emits tokens. Before lexing, the function uses the right input stream (either the source code or `AINSERT` buffer if not empty). After choosing the right input source, the lexer emits token. HLASM introduces *continuation* symbol (an arbitrary non-blank symbol in the continuation column) that breaks one logical line into two or more source-code lines. The end of one logical line indicates `EOLLN` token. Such token is important for further (syntactic and semantic) analysis.

- `create_token()`  
creates token of given type. The lexer’s internal state gives the position of the token.

- `consume()`  
consumes character from the input stream and updates lexer’s internal state (used in `create_token()`).

- `lex_tokens()`  
lexing of most of the token types.

- `lex_begin()`  
up to certain column, the input can be ignored (can be set in HLASM).

- `lex_end()`  
lexes everything after continuation symbol.
