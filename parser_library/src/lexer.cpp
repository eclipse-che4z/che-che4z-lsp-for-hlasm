#include "../include/shared/lexer.h"
#include  <utility>
#include <cctype>
#include <algorithm>
#include <string>
#include <assert.h>


using namespace antlr4;
using namespace std;

using namespace hlasm_plugin;
using namespace parser_library;

lexer::lexer(CharStream* input)
	: input_(input)
{
	factory_ = std::make_unique<token_factory>();
	ainsert_stream_ = make_unique<input_source>("");

	file_input_state_.input = input;
	buffer_input_state_.input = ainsert_stream_.get();

	input_state_->c = static_cast<char_t>(input_->LA(1));
}

void lexer::set_unlimited_line(bool ul)
{
	unlimited_line_ = ul;
}

bool lexer::get_unlimited_line() const
{
	return unlimited_line_;
}

size_t lexer::getLine() const
{
	return input_state_->line;
}

size_t lexer::getCharPositionInLine()
{
	return input_state_->char_position_in_line;
}

antlr4::CharStream* lexer::getInputStream()
{
	return input_;
}

std::string lexer::getSourceName()
{
	return input_->getSourceName();
}

bool lexer::double_byte_enabled() const
{
	return double_byte_enabled_;
}

void lexer::set_double_byte_enabled(bool dbe)
{
	double_byte_enabled_ = dbe;
}

/*
 * check if token is after continuation
 * token is unmarked after the call
 */
bool lexer::continuation_before_token(size_t token_id)
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
	if (ttype == Token::EOF)
	{
		assert(!eof_generated);
		eof_generated = true;
	}
	/* do not generate empty tokens (except EOF and EOLLN */
	if (token_start_state_.char_position == token_start_state_.input->index()
		&& ttype != Token::EOF
		&& ttype != EOLLN)
		return;

	/* mark first token after continuation */
	if (channel == DEFAULT_CHANNEL && last_continuation_ != static_cast<size_t>(-1))
	{
		last_continuation_ = static_cast<size_t>(-1);
		tokens_after_continuation_.emplace(last_token_id_);
	}

	/* record last continuation */
	if (ttype == CONTINUATION)
		last_continuation_ = last_token_id_;

	last_token_id_++;

	token_queue_.push(factory_->create(
		this,
		token_start_state_.input,
		ttype,
		channel,
		token_start_state_.char_position,
		input_state_->char_position - 1,
		token_start_state_.line,
		token_start_state_.char_position_in_line,
		last_token_id_ - 1,
		token_start_state_.char_position_in_line_utf16));
}

void lexer::consume()
{
	if (input_state_->c == '\n')
	{
		input_state_->line++;
		input_state_->char_position_in_line = static_cast<size_t>(-1);
		input_state_->char_position_in_line_utf16 = static_cast<size_t>(-1);
	}

	if (input_state_->c != static_cast<char_t>(-1))
	{
		input_state_->input->consume();
		input_state_->char_position++;
		input_state_->c = static_cast<char_t>(input_state_->input->LA(1));

		if (input_state_->c == '\t')
		{
			input_state_->c = ' ';
			input_state_->char_position_in_line += tab_size_;
			input_state_->char_position_in_line_utf16 += tab_size_;
		}
		else
		{
			input_state_->char_position_in_line++;
			input_state_->char_position_in_line_utf16 += input_state_->c > 0xFFFF ? 2 : 1;
		}
	}
}

bool lexer::from_buffer() const
{
	return &buffer_input_state_ == input_state_;
}

bool lexer::eof() const
{
	return input_->LA(1) == CharStream::EOF
		&& ainsert_stream_->LA(1) == CharStream::EOF;
}

/* set start token info */
void lexer::start_token()
{
	token_start_state_ = *input_state_;
}

void lexer::switch_input_streams()
{
	if (!ainsert_buffer_.empty())
	{
		UTF32String f = ainsert_buffer_.front();
		ainsert_stream_->append(f);
		ainsert_buffer_.pop_front();
		if (input_state_->input != ainsert_stream_.get())
		{
			input_state_ = &buffer_input_state_;
		}
		input_state_->c = static_cast<char_t>(input_state_->input->LA(1));
	}

	if (ainsert_stream_->LA(1) == ainsert_stream_->EOF)
		input_state_ = &file_input_state_;
}

token_ptr lexer::nextToken()
{
	while (true) {

		if (!token_queue_.empty())
		{
			auto t = move(token_queue_.front());
			token_queue_.pop();
			return t;
		}

		switch_input_streams();


		start_token();

		if (eof())
		{
			create_token(EOLLN);
			create_token(Token::EOF);
		}

		else if (double_byte_enabled_)
			check_continuation();

		else if (!unlimited_line_ 
			&& input_state_->char_position_in_line == end_ 
			&& !isspace(input_state_->c) 
			&& continuation_enabled_
			&& (!from_buffer() || !ainsert_buffer_.empty()))
			lex_continuation();

		else if (
			(unlimited_line_ && (input_state_->c == '\r' || input_state_->c == '\n'))
			|| (!unlimited_line_ && input_state_->char_position_in_line >= end_))
			lex_end(true);

		else if (input_state_->char_position_in_line < begin_)
			lex_begin();

		else
			lex_tokens();
	}
}

void lexer::lex_tokens()
{

	switch (input_state_->c)
	{
	case '*':
		if (input_state_->char_position_in_line == begin_)
		{
			if (is_process()) {
				lex_process();
				break;
			}
			lex_comment();
		}
		else
		{
			consume();
			create_token(ASTERISK);
		}
		break;

	case '.':
		/* macro comment */
		if (input_state_->char_position_in_line == begin_ && input_->LA(2) == '*') {
			lex_comment();
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

	case '\r':
		consume();
		if (input_state_->c == '\n')
			consume();
		create_token(EOLLN);
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
	switch (input_state_->c)
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
	while (input_state_->char_position_in_line < begin_ && !eof())
		consume();
	create_token(IGNORED, HIDDEN_CHANNEL);
}

void lexer::lex_end(bool eolln)
{
	start_token();
	while (input_state_->c != '\n' && !eof() && input_state_->c != static_cast<size_t>(-1))
		consume();
	if (!eof())
	{
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
		while (input_state_->char_position_in_line < end_ && !eof() && input_state_->c != '\n')
			consume();
		create_token(COMMENT, HIDDEN_CHANNEL);

		if (!isspace(input_state_->c) && !eof() && continuation_enabled_)
			lex_continuation();
		else
		{
			lex_end(false);
			break;
		}
	}
}

void lexer::consume_new_line()
{
	if (input_state_->c == '\r')
		consume();
	if (input_state_->c == '\n')
		consume();
}

/* lex continuation and ignores */
void lexer::lex_continuation()
{
	start_token();

	/* lex continuation */
	while (input_state_->char_position_in_line <= end_default_ && !eof())
		consume();

	/* reset END */
	end_ = end_default_;

	create_token(CONTINUATION, HIDDEN_CHANNEL);

	lex_end(false);
	lex_begin();

	/* lex continuation */
	start_token();
	while (input_state_->char_position_in_line < continue_ && ! eof())
		consume();
	create_token(CONTINUATION, HIDDEN_CHANNEL);
}

/* if DOUBLE_BYTE_ENABLED check start of continuation for current line */
void lexer::check_continuation()
{
	end_ = end_default_;

	if (from_buffer() && ainsert_buffer_.empty())
		return;

	auto cc = input_->LA(end_default_ + 1);
	if (cc != CharStream::EOF && !isspace(static_cast<int>(cc)))
	{
		do
		{
			if (input_->LA(end_) != cc)
				break;
			end_--;
		} while (end_ > begin_);
	}
}

void lexer::lex_space()
{
	while (input_state_->c == ' ' && before_end() && !eof())
		consume();
	create_token(SPACE, DEFAULT_CHANNEL);
}

bool lexer::before_end() const
{
	return (input_state_->char_position_in_line < end_
		|| (unlimited_line_ && input_state_->c != '\r' && input_state_->c != '\n')
		|| (input_state_->char_position_in_line == end_
			&& from_buffer() && !ainsert_buffer_.empty())
		);
}

bool lexer::is_ord_char() const
{
	return isalnum(input_state_->c) || isalpha(input_state_->c)
		|| input_state_->c == '_' || input_state_->c == '@'
		|| input_state_->c == '$' || input_state_->c == '#';
}

void lexer::lex_word()
{
	bool ord = is_ord_char() && (input_state_->c < '0' || input_state_->c > '9');

	size_t w_len = 0;
	while (!isspace(input_state_->c) && !eof() && !identifier_divider()
		&& before_end())
	{
		++w_len;
		ord &= is_ord_char();
		consume();
	}

	if (ord && w_len <= 63)
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
	return ((ictl_ && input_state_->line <= 11) || (!ictl_ && input_state_->line <= 10))
		&& toupper(static_cast<int>(input_state_->input->LA(2))) == 'P'
		&& toupper(static_cast<int>(input_state_->input->LA(3))) == 'R'
		&& toupper(static_cast<int>(input_state_->input->LA(4))) == 'O'
		&& toupper(static_cast<int>(input_state_->input->LA(5))) == 'C'
		&& toupper(static_cast<int>(input_state_->input->LA(6))) == 'E'
		&& toupper(static_cast<int>(input_state_->input->LA(7))) == 'S'
		&& toupper(static_cast<int>(input_state_->input->LA(8))) == 'S';
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
	while (!eof() && before_end() && input_state_->c != '\n' && (apostrophes_ % 2 == 1 || (apostrophes_ % 2 == 0 && input_state_->c != ' '))) {
		start_token();
		lex_tokens();
	}
	end_--;
	lex_end(true);
}


bool lexer::char_start_utf8(unsigned c)
{
	/* https://en.wikipedia.org/wiki/UTF-8#Description */
	return !(c & (1 << 7))

		|| (!(c & (1 << 5)) && (c & (3 << 6)))

		|| (!(c & (1 << 4)) && (c & (7 << 5)))

		|| (!(c & (1 << 3)) && (c & (15 << 4)));
}

size_t lexer::length_utf16(const std::string& str)
{
	if (str.length() == 0)
		return 0;

	size_t l = 0;
	size_t ll = 1;
	for (auto i = str.cbegin() + 1; i != str.cend(); ++i)
	{
		if (char_start_utf8(*i))
		{
			/* 4B in UTF8 = 2x UTF16 otherwise 1x UTF16 */
			l += (ll == 4) ? 2 : 1;
			ll = 0;
		}
		++ll;
	}
	l += (ll == 4) ? 2 : 1;
	return l;
}

std::string lexer::aread()
{
	string str;

	switch_input_streams();

	start_token();

	while (!eof() && input_state_->c != '\n' && input_state_->c != static_cast<char_t>(-1) && str.length() < 80)
	{
		str.append(cvt_.to_bytes(input_state_->c));
		consume();
	}
	create_token(AREAD, HIDDEN_CHANNEL);
	lex_end(false);
	return str;
}

std::unique_ptr<input_source>& lexer::get_ainsert_stream()
{
	return ainsert_stream_;
}

void lexer::ainsert_back(const std::string& back)
{
	ainsert(back, true);
}

void lexer::ainsert_front(const std::string& back)
{
	ainsert(back, false);
}

void lexer::ainsert(const std::string & inp, bool front)
{
	auto len = length_utf16(inp);
	auto s = cvt_.from_bytes(inp);
	UTF32String str(s.begin(), s.end());
	if (len > 0)
	{
		for (; len < 80; ++len)
			str.push_back(' ');
		str.push_back('\n');
		if (front)
			ainsert_buffer_.push_front(str);
		else
			ainsert_buffer_.push_back(str);
	}
	else
	{
		throw std::runtime_error("invalid insertion - empty string");
	}
}
