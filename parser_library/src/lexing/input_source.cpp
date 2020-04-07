/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#include "input_source.h"
#include <exception>

namespace hlasm_plugin::parser_library::lexing
{


input_source::input_source(const std::string& input)
	:ANTLRInputStream(input) {}

void input_source::append(const UTF32String& str)
{
	_data.append(str);
}

void input_source::append(const std::string& str)
{
	p = _data.size();
	_data.append(antlrcpp::utf8_to_utf32(str.data(), str.data() + str.size()));
}

void input_source::reset(const std::string& str)
{
	load(str);
}

std::string hlasm_plugin::parser_library::input_source::getText(const antlr4::misc::Interval& interval)
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
			throw std::runtime_error("not valid unicode character");

	}
	return n;


}

void input_source::rewind_input(size_t position)
{
	assert(position < _data.length());
	p = position;
}

}