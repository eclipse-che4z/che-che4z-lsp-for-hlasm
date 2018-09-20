#include "../include/shared/input_source.h"

using namespace hlasm_plugin;
using namespace parser_library;

input_source::input_source(const std::string & input)
	:ANTLRInputStream(input)
{
	
}


void input_source::append(const UTF32String &str)
{
	_data.append(str);
}

