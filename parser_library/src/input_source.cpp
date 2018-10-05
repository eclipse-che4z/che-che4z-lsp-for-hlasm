#include "shared/input_source.h"

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

std::string hlasm_plugin::parser_library::input_source::getText(const antlr4::misc::Interval & interval)
{
		std::string n;
		for (auto i = interval.a; i <= interval.b; ++i)
		{
			uint32_t ch = _data[i];
			constexpr unsigned int last6 = 0x3F;
			constexpr unsigned int last5 = 0x1F;
			constexpr unsigned int last4 = 0x0F;
			constexpr unsigned int last3 = 0x07;
			if (ch <= 0x007F) //U+0000 - U+007F
				n.insert(n.end(), (char)ch);
			else if (ch <= 0x7FF) //U+0080 - U+07FF
			{
				char utf8[2];
				utf8[1] = '\x80'; //10xxxxxx
				utf8[1] += ch & last6;

				ch = ch >> 6;
				utf8[0] = '\xc0'; //110xxxxx
				utf8[0] += ch & last5;

				n.append(utf8, 2);
			}
			else if (ch <= 0xFFFF) //U+0800 - U+FFFF
			{
				char utf8[3];
				//third
				utf8[2] = '\x80';
				utf8[2] += ch & last6;

				ch = ch >> 6;
				//second
				utf8[1] = '\x80';
				utf8[1] += ch & last6;

				ch = ch >> 6;
				utf8[0] = '\xe0';
				utf8[0] += ch & last4;

				n.append(utf8, 3);
			}
			else if (ch <= 0x10FFFF)//U+10000 - U+10FFFF
			{
				char utf8[4];
				for (int j = 3; j > 0; --j)
				{
					utf8[j] = '\x80';
					utf8[j] += ch & last6;
					ch = ch >> 6;
				}

				utf8[0] = '\xf0'; //11110xxx
				utf8[0] += ch & last3;

				n.append(utf8, 4);
			}
			else
				throw std::exception("not valid unicode character");

		}
		return n;
	

}

void input_source::rewind_input(size_t position)
{
	assert(position < _data.length());
	p = position;
}
