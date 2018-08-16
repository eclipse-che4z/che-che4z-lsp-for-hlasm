#include "../include/shared/lexer.h"
#include  <utility>
#include <cctype>
#include <iostream>
#include <algorithm>


using namespace antlr4;
using namespace std;

using namespace hlasm_plugin;
using namespace parser_library;

std::map<std::basic_string_view<char_t>, lexer::Tokens> words = {
		{ U"OR", lexer::OR},
		{ U"AND", lexer::AND},
		{ U"EQ", lexer::EQ},
		{ U"LE", lexer::LE},
		{ U"LT", lexer::LTx},
		{ U"GT", lexer::GTx},
		{ U"GE", lexer::GE}
};

lexer::lexer(CharStream* input)
	: input_(input)
{
	current_word_.reserve(80);
	c_ = input->LA(1);
}

size_t lexer::getLine() const
{
	return line_;
}

size_t lexer::getCharPositionInLine()
{
	return char_position_in_line_;
}

antlr4::CharStream* lexer::getInputStream()
{
	return input_;
}

std::string lexer::getSourceName()
{
	return input_->getSourceName();
}

Ref<antlr4::TokenFactory<antlr4::CommonToken>>
lexer::getTokenFactory()
{
	return factory_;
}

bool lexer::doubleByteEnabled() const
{
	return double_byte_enabled_;
}

void lexer::setDoubleByteEnabled(bool dbe)
{
	double_byte_enabled_ = dbe;
}

/*
 * check if token is after continuation
 * token is unmarked after the call
 */
bool lexer::continuationBeforeToken(size_t token_id)
{
	if (tokens_after_continuation_.find(token_id) != tokens_after_continuation_.end())
	{
		tokens_after_continuation_.erase(token_id);
		return true;
	}
	return false;
}

void lexer::create_token(size_t ttype, size_t channel = Channels::DEFAULT_CHANNEL)
{
	/* do not generate empty tokens (except EOF and EOLLN */
	if (start_char_index_ == input_->index() && ttype != Token::EOF && ttype != EOLLN)
		return;

	auto source = make_pair(this, input_);
	auto text = "";

	/* mark first token after continuation */
	if (channel == Channels::DEFAULT_CHANNEL && last_continuation_ != -1)
	{
		last_continuation_ = static_cast<size_t>(-1);
		tokens_after_continuation_.emplace(last_token_id_);
	}

	/* record last continuation */
	if (ttype == CONTINUATION)
		last_continuation_ = last_token_id_;

	last_token_id_++;

	token_queue_.push(factory_->create(source, ttype, text, channel,
		start_char_index_, input_->index() - 1,
		start_line_, start_char_position_in_line_));
}

void lexer::consume()
{
	if (c_ == '\n')
	{
		line_++;
		char_position_in_line_ = static_cast<size_t>(-1);
	}

	if (c_ != CharStream::EOF)
		input_->consume();

	c_ = (char_t)input_->LA(1);
	if (c_ == '\t')
		char_position_in_line_ += tab_size_;
	else
		char_position_in_line_++;
}

bool lexer::eof() const
{
	return c_ == CharStream::EOF;
}

/* set start token info */
void lexer::start_token()
{
	start_char_index_ = input_->index();
	start_line_ = line_;
	start_char_position_in_line_ = char_position_in_line_;
}

token_ptr lexer::nextToken()
{
	while (true) {
		start_token();

		if (!token_queue_.empty())
		{
			auto t = move(token_queue_.front());
			token_queue_.pop();
			return t;
		}

		if (eof())
		{
			create_token(EOLLN);
			create_token(Token::EOF);
			continue;
		}

		else if (double_byte_enabled_)
			check_continuation();

		else if (char_position_in_line_ == end_ && !isspace(c_) && continuation_enabled_)
			lex_continuation();

		else if (char_position_in_line_ >= end_)
			lex_end(true);

		else if (char_position_in_line_ < begin_)
			lex_begin();

		else {
			lex_tokens();
		}
	}
}

void lexer::lex_tokens()
{

	switch (c_)
	{
	case '*':
		if (char_position_in_line_ == begin_)
		{
			if (is_process()) {
				lex_process();
				break;
			}
			lex_comment();
			break;
		}
		else
		{
			consume();
			create_token(ASTERISK);
		}
		break;

	case '.':
		/* macro comment */
		if (char_position_in_line_ == begin_ && input_->LA(2) == '*') {
			lex_comment();
			break;
		}
		else {
			consume();
			create_token(DOT);
		}
		break;

	case ' ':
		lex_space();
		break;

	case '-':
		consume();
		create_token(MINUS);
		break;

	case '+':
		consume();
		create_token(PLUS);
		break;

	case '=':
		consume();
		create_token(EQUALS);
		break;

	case '<':
		consume();
		create_token(LT);
		break;

	case '>':
		consume();
		create_token(GT);
		break;

	case ',':
		consume();
		create_token(COMMA);
		break;

	case '(':
		consume();
		create_token(LPAR);
		break;

	case ')':
		consume();
		create_token(RPAR);
		break;

	case '\'':
		apostrophes_++;
		consume();
		create_token(APOSTROPHE);
		break;

	case '/':
		consume();
		create_token(SLASH);
		break;

	case '&':
		consume();
		create_token(AMPERSAND);
		break;

	case '\n':
		consume();
		create_token(EOLLN);
		break;

	case '|':
		consume();
		create_token(VERTICAL);
		break;

	default:
		lex_word();
		break;
	}
}

bool lexer::identifier_divider() const
{
	switch (c_)
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

void lexer::lex_begin()
{
	start_token();
	while (char_position_in_line_ < begin_)
		consume();
	create_token(IGNORED, HIDDEN_CHANNEL);
}

void lexer::lex_end(bool eolln)
{
	start_token();
	while (c_ != '\n' && !eof())
		consume();
	if (!eof()) {
		consume();
		if (eolln)
			create_token(EOLLN);
	}
	if (double_byte_enabled_)
		check_continuation();
	create_token(IGNORED, HIDDEN_CHANNEL);
}

void lexer::lex_comment()
{
	while (true)
	{

		start_token();
		while (char_position_in_line_ < end_ && !eof() && c_ != '\n')
			consume();
		create_token(COMMENT, HIDDEN_CHANNEL);

		if (!isspace(c_) && !eof() && continuation_enabled_)
			lex_continuation();
		else {
			consume_new_line();
			break;
		}
	}
}

void lexer::consume_new_line()
{
	if (c_ == '\r')
		consume();
	if (c_ == '\n')
		consume();
}

/* lex continuation and ignores */
void lexer::lex_continuation()
{
	start_token();

	/* lex continuation */
	while (char_position_in_line_ <= end_default_)
		consume();

	/* reset END */
	end_ = end_default_;

	create_token(CONTINUATION, HIDDEN_CHANNEL);

	lex_end(false);
	lex_begin();

	/* lex continuation */
	start_token();
	while (char_position_in_line_ < continue_)
		consume();
	create_token(CONTINUATION, HIDDEN_CHANNEL);
}

/* if DOUBLE_BYTE_ENABLED check start of continuation for current line */
void lexer::check_continuation()
{
	auto cc = input_->LA(end_default_ + 1);
	end_ = end_default_;
	if (cc != CharStream::EOF && !isspace(cc)) {
		do {
			if (input_->LA(end_) != cc)
				break;
			end_--;
		} while (end_ > begin_);
	}
}

void lexer::lex_space()
{
	while (c_ == ' ' && char_position_in_line_ < end_)
		consume();
	create_token(SPACE, DEFAULT_CHANNEL);
}

void lexer::lex_word()
{
	bool ord = isalpha(c_);

	current_word_.clear();
	while (!isspace(c_) && !eof() && !identifier_divider() && char_position_in_line_ < end_)
	{
		current_word_.push_back(toupper(c_));
		ord &= isalnum(c_) || isalpha(c_);
		consume();
	}

	if (current_word_.length() < 4) {
		auto f = words.find(current_word_);
		if (f != words.end()) {
			return create_token(f->second);
		}
	}

	if (ord && current_word_.length() <= 63)
		create_token(ORDSYMBOL);
	else
		create_token(IDENTIFIER);
}

bool lexer::set_begin(size_t begin)
{
	if (begin >= 1 && begin <= 40) {
		begin_ = begin;
		return true;
	}
	return false;
}

bool lexer::set_end(size_t end)
{
	if (end == 80)
		continuation_enabled_ = false;
	if (end >= 41 && end <= 80) {
		end_default_ = end;
		end_ = end_default_;
		return true;
	}
	return false;
}

bool lexer::set_continue(size_t cont)
{
	if (cont >= 2 && cont <= 40 && begin_ < cont) {
		continue_ = cont;
		return true;
	}
	return false;
}

void lexer::set_continuation_enabled(bool enabled)
{
	continuation_enabled_ = enabled;
}

bool lexer::is_process() const
{
	if (
		((ictl_ && line_ <= 11) || (!ictl_ && line_ <= 10))
		&& toupper(input_->LA(2)) == 'P'
		&& toupper(input_->LA(3)) == 'R'
		&& toupper(input_->LA(4)) == 'O'
		&& toupper(input_->LA(5)) == 'C'
		&& toupper(input_->LA(6)) == 'E'
		&& toupper(input_->LA(7)) == 'S'
		&& toupper(input_->LA(8)) == 'S')
		return true;
	return false;
}

void lexer::set_ictl()
{
	ictl_ = true;
}

void lexer::lex_process()
{
	/* lex *PROCESS */
	start_token();
	for (size_t i = 0; i < 8; i++)
		consume();
	create_token(PROCESS);

	start_token();
	lex_space();

	apostrophes_ = 0;
	end_++; /* including END column */
	while (!eof() && char_position_in_line_ < end_ && c_ != '\n' && (apostrophes_ % 2 == 1 || (apostrophes_ % 2 == 0 && c_ != ' '))) {
		start_token();
		lex_tokens();
	}
	end_--;
	lex_end(true);
}
