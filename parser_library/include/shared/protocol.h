#ifndef HLASMPLUGIN_PARSERLIBRARY_PROTOCOL_H
#define HLASMPLUGIN_PARSERLIBRARY_PROTOCOL_H

#include <cstring>

#include "../src/generated/parser_library_export.h"

namespace hlasm_plugin::parser_library {

using version_t = uint64_t;
using location_t = uint64_t;

struct PARSER_LIBRARY_EXPORT location
{
	location() : line(0), column(0) {}
	location(location_t line, location_t column) : line(line), column(column) {}
	bool operator==(const location & l) const
	{
		return line == l.line && column == l.column;
	}
	location_t line;
	location_t column;
};

struct PARSER_LIBRARY_EXPORT range
{
	range() {}
	range(location start, location end) : start(start), end(end) {}
	bool operator==(const range & r) const
	{
		return start == r.start && end == r.end;
	}
	location start;
	location end;
};

struct PARSER_LIBRARY_EXPORT document_change
{
	document_change(const char * new_text, size_t text_length) : 
		whole(true), text(new_text), text_length(text_length) {}
	document_change(range change_range, const char * new_text, size_t text_length) :
		whole(false), change_range(change_range), text(new_text), text_length(text_length) {}

	bool operator==(const document_change & ch) const
	{

		return whole == ch.whole && change_range == ch.change_range &&
			text_length == ch.text_length && std::memcmp(text, ch.text, text_length) == 0;
	}

	const bool whole;
	const range change_range;
	const char * const text;
	const size_t text_length;
};

struct PARSER_LIBRARY_EXPORT text_document_item
{
	char * document_uri;
	version_t version;
	char * text;
};

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_PROTOCOL_H
