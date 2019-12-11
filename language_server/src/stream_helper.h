#ifndef HLASMPLUGIN_LANGUAGESERVER_STREAM_HELPER_H
#define HLASMPLUGIN_LANGUAGESERVER_STREAM_HELPER_H

#include <locale>
#include <iostream>

namespace hlasm_plugin::language_server
{

struct newline_is_space : std::ctype<char> {
	newline_is_space() : std::ctype<char>(get_table()) {}
	static mask const* get_table()
	{
		static mask rc[table_size];
		rc[(unsigned char)'\n'] = std::ctype_base::space;
		return rc;
	}

	static void imbue_stream(std::ios& stream)
	{
		stream.imbue(std::locale(stream.getloc(), new newline_is_space));
	}
};

}

#endif