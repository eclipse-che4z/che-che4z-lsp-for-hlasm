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

#ifndef HLASMPLUGIN_LANGUAGESERVER_STREAM_HELPER_H
#define HLASMPLUGIN_LANGUAGESERVER_STREAM_HELPER_H

#include <locale>
#include <iostream>

namespace hlasm_plugin::language_server
{

//A struct that can be imbued into std::iostream, so it recognizes 
//only newline and nothing else as a whitespace. The dispatcher
//expects a stream like that in the constructor.
struct newline_is_space : std::ctype<char>
{
	newline_is_space() : std::ctype<char>(get_table()) {}
	static mask const* get_table()
	{
		static mask rc[table_size];
		rc[(unsigned char) '\n'] = std::ctype_base::space;
		return rc;
	}

	static void imbue_stream(std::ios& stream)
	{
		stream.imbue(std::locale(stream.getloc(), new newline_is_space));
	}
};

}

#endif
