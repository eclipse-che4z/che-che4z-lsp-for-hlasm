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

#ifndef HLASMPLUGIN_PARSER_HLASMEBCDIC_H
#define HLASMPLUGIN_PARSER_HLASMEBCDIC_H

#include  <string>

namespace hlasm_plugin {
namespace parser_library {

class ebcdic_encoding
{
public:
	static constexpr unsigned char SUB = 26;

	static unsigned char a2e[256];

	static unsigned char e2a[256];

	static unsigned char to_pseudoascii(const char*& c);

	static unsigned char to_ebcdic(unsigned char c);

	static std::string to_ebcdic(const std::string& s);

	static std::string to_ascii(unsigned char c);

	static std::string to_ascii(const std::string& s);
};

inline char operator ""_ebcdic(char c)
{
	return ebcdic_encoding::a2e[c];
}

}
}

#endif
