#include "../include/shared/HlasmLexer.h"
#include  <utility>
#include <cctype>
#include <iostream>
#include <algorithm>


using namespace antlr4;
using namespace std;

using namespace HlasmPlugin;
using namespace HlasmParserLibrary;

std::map<std::basic_string_view<char_t>, HlasmLexer::Tokens> words = {
		{ U"OR", HlasmLexer::OR},
		{ U"AND", HlasmLexer::AND},
		{ U"EQ", HlasmLexer::EQ},
		{ U"LE", HlasmLexer::LE},
		{ U"LT", HlasmLexer::LTx},
		{ U"GT", HlasmLexer::GTx},
		{ U"GE", HlasmLexer::GE}
};

HlasmLexer::HlasmLexer(CharStream* input)
	: input(input)
{
	current_word.reserve(80);
	c = input->LA(1);
}

size_t HlasmLexer::getLine() const
{
	return line;
}

size_t HlasmLexer::getCharPositionInLine()
{
	return charPositionInLine;
}

antlr4::CharStream* HlasmLexer::getInputStream()
{
	return input;
}

std::string HlasmLexer::getSourceName()
{
	return input->getSourceName();
}

Ref<antlr4::TokenFactory<antlr4::CommonToken>>
HlasmLexer::getTokenFactory()
{
	return factory;
}

bool HlasmLexer::doubleByteEnabled() const {
	return DOUBLE_BYTE_ENABLED;
}

void HlasmLexer::setDoubleByteEnabled(bool dbe) {
	DOUBLE_BYTE_ENABLED = dbe;
}

/*
 * check if token is after continuation
 * token is unmarked after the call
 */
bool HlasmLexer::continuationBeforeToken(size_t token_id) {
	if (tokens_after_continueation.find(token_id) != tokens_after_continueation.end()) {
		tokens_after_continueation.erase(token_id);
		return true;
	}
	return false;
}

void HlasmLexer::createToken(size_t ttype, size_t channel = Channels::DEFAULT_CHANNEL)
{
	/* do not generate empty tokens (except EOF and EOLLN */
	if (startCharIndex == input->index() && ttype != Token::EOF && ttype != EOLLN)
		return;

	auto source = make_pair(this, input);
	auto text = "";

	/* mark first token after continuation */
	if (channel == Channels::DEFAULT_CHANNEL && last_continuation != -1) {
		last_continuation = static_cast<size_t>(-1);
		tokens_after_continueation.emplace(last_token_id);
	}

	/* record last continuation */
	if (ttype == CONTINUATION)
		last_continuation = last_token_id;

	last_token_id++;

	token_queue.push(factory->create(source, ttype, text, channel,
		startCharIndex, input->index() - 1,
		startLine, startCharPositionInLine));
}

void HlasmLexer::consume() {
	if (c == '\n') {
		line++;
		charPositionInLine = static_cast<size_t>(-1);
	}

	if (c != CharStream::EOF)
		input->consume();

	c = (char_t)input->LA(1);
	if (c == '\t')
		charPositionInLine += TAB_SIZE;
	else
		charPositionInLine++;
}

bool HlasmLexer::EOF() const
{
	return c == CharStream::EOF;
}

/* set start token info */
void HlasmLexer::startToken()
{
	startCharIndex = input->index();
	startLine = line;
	startCharPositionInLine = charPositionInLine;
}

token_ptr HlasmLexer::nextToken()
{
	while (true) {
		startToken();

		if (!token_queue.empty())
		{
			auto t = move(token_queue.front());
			token_queue.pop();
			return t;
		}

		if (EOF()) {
			createToken(EOLLN);
			createToken(Token::EOF);
			continue;
		}

		else if (DOUBLE_BYTE_ENABLED)
			checkContinuation();

		else if (charPositionInLine == END && !isspace(c) && CONTINUATION_ENABLED)
			lexContinuation();

		else if (charPositionInLine >= END)
			lexEnd(true);

		else if (charPositionInLine < BEGIN)
			lexBegin();

		else {
			lexTokens();
		}
	}
}

void HlasmLexer::lexTokens() {

	switch (c)
	{
	case '*':
		if (charPositionInLine == BEGIN) {
			if (isProcess()) {
				lexProcess();
				break;
			}
			lexComment();
			break;
		}
		else {
			consume();
			createToken(ASTERISK);
		}
		break;

	case '.':
		/* macro comment */
		if (charPositionInLine == BEGIN && input->LA(2) == '*') {
			lexComment();
			break;
		}
		else {
			consume();
			createToken(DOT);
		}
		break;

	case ' ':
		lexSpace();
		break;

	case '-':
		consume();
		createToken(MINUS);
		break;

	case '+':
		consume();
		createToken(PLUS);
		break;

	case '=':
		consume();
		createToken(EQUALS);
		break;

	case '<':
		consume();
		createToken(LT);
		break;

	case '>':
		consume();
		createToken(GT);
		break;

	case ',':
		consume();
		createToken(COMMA);
		break;

	case '(':
		consume();
		createToken(LPAR);
		break;

	case ')':
		consume();
		createToken(RPAR);
		break;

	case '\'':
		apostrophes++;
		consume();
		createToken(APOSTROPHE);
		break;

	case '/':
		consume();
		createToken(SLASH);
		break;

	case '&':
		consume();
		createToken(AMPERSAND);
		break;

	case '\n':
		consume();
		createToken(EOLLN);
		break;

	case '|':
		consume();
		createToken(VERTICAL);
		break;

	default:
		lexWord();
		break;
	}
}

bool HlasmLexer::identifierDivider() const
{
	switch (c)
	{
	case '*':
	case '.':
	case '-':
	case '+':
	case '=':
	case '<':
	case '>':
	case ',':
	case '(':
	case ')':
	case '\'':
	case '/':
	case '&':
	case '|':
		return true;
	default:
		return false;
	}
}

void HlasmLexer::lexBegin() {
	startToken();
	while (charPositionInLine < BEGIN)
		consume();
	createToken(IGNORED, HIDDEN_CHANNEL);
}

void HlasmLexer::lexEnd(bool eolln) {
	startToken();
	while (c != '\n' && !EOF())
		consume();
	if (!EOF()) {
		consume();
		if (eolln)
			createToken(EOLLN);
	}
	if (DOUBLE_BYTE_ENABLED)
		checkContinuation();
	createToken(IGNORED, HIDDEN_CHANNEL);
}

void HlasmLexer::lexComment() {
	while (true) {

		startToken();
		while (charPositionInLine < END && !EOF() && c != '\n')
			consume();
		createToken(COMMENT, HIDDEN_CHANNEL);

		if (!isspace(c) && !EOF() && CONTINUATION_ENABLED)
			lexContinuation();
		else {
			consumeNewLine();
			break;
		}
	}
}

void HlasmLexer::consumeNewLine() {
	if (c == '\r')
		consume();
	if (c == '\n')
		consume();
}

/* lex continuation and ignores */
void HlasmLexer::lexContinuation() {
	startToken();

	/* lex continuation */
	while (charPositionInLine <= END_DEFAULT)
		consume();

	/* reset END */
	END = END_DEFAULT;

	createToken(CONTINUATION, HIDDEN_CHANNEL);

	lexEnd(false);
	lexBegin();

	/* lex continuation */
	startToken();
	while (charPositionInLine < CONTINUE)
		consume();
	createToken(CONTINUATION, HIDDEN_CHANNEL);
}

/* if DOUBLE_BYTE_ENABLED check start of continuation for current line */
void HlasmLexer::checkContinuation() {
	auto cc = input->LA(END_DEFAULT + 1);
	END = END_DEFAULT;
	if (cc != CharStream::EOF && !isspace(cc)) {
		do {
			if (input->LA(END) != cc)
				break;
			END--;
		} while (END > BEGIN);
	}
}

void HlasmLexer::lexSpace() {
	while (c == ' ' && charPositionInLine < END)
		consume();
	createToken(SPACE, DEFAULT_CHANNEL);
}

void HlasmLexer::lexWord() {
	bool ord = isalpha(c);

	while (!isspace(c) && !EOF() && !identifierDivider() && charPositionInLine < END)
	{
		current_word.push_back(toupper(c));
		ord &= isalnum(c) || isalpha(c);
		consume();
	}

	if (current_word.length() < 4) {
		auto f = words.find(current_word);
		if (f != words.end()) {
			return createToken(f->second);
		}
	}

	if (ord && current_word.length() <= 63)
		createToken(ORDSYMBOL);
	else
		createToken(IDENTIFIER);
}

bool HlasmLexer::setBegin(size_t begin) {
	if (begin >= 1 && begin <= 40) {
		BEGIN = begin;
		return true;
	}
	return false;
}

bool HlasmLexer::setEnd(size_t end) {
	if (end == 80)
		CONTINUATION_ENABLED = false;
	if (end >= 41 && end <= 80) {
		END_DEFAULT = end;
		END = END_DEFAULT;
		return true;
	}
	return false;
}

bool HlasmLexer::setContinue(size_t cont) {
	if (cont >= 2 && cont <= 40 && BEGIN < cont) {
		CONTINUE = cont;
		return true;
	}
	return false;
}

void HlasmLexer::setContinuationEnabled(bool enabled)
{
	CONTINUATION_ENABLED = enabled;
}

bool HlasmLexer::isProcess() const
{
	if (
		((ICTL && line <= 11) || (!ICTL && line <= 10))
		&& toupper(input->LA(2)) == 'P'
		&& toupper(input->LA(3)) == 'R'
		&& toupper(input->LA(4)) == 'O'
		&& toupper(input->LA(5)) == 'C'
		&& toupper(input->LA(6)) == 'E'
		&& toupper(input->LA(7)) == 'S'
		&& toupper(input->LA(8)) == 'S')
		return true;
	return false;
}

void HlasmLexer::setICTL() {
	ICTL = true;
}

void HlasmLexer::lexProcess() {
	/* lex *PROCESS */
	startToken();
	for (size_t i = 0; i < 8; i++)
		consume();
	createToken(PROCESS);

	startToken();
	lexSpace();

	apostrophes = 0;
	END++; /* including END column */
	while (!EOF() && charPositionInLine < END && c != '\n' && (apostrophes % 2 == 1 || (apostrophes % 2 == 0 && c != ' '))) {
		startToken();
		lexTokens();
	}
	END--;
	lexEnd(true);
}
