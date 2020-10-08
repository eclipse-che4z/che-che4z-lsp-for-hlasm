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

#include <cstring>
#include <string>

#include "analyzer.h"
#include "workspaces/file_impl.h"

using namespace hlasm_plugin::parser_library;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::string source;
    source.resize(size);
    memcpy((void*)source.data(), data, size);

    for (char c : source)
    {
        if (c <= 8 || c == '\t' || c == 11 || c == 12 || (c >= 14 && c < 32) || c >= 127)
        {
            // std::cout << "invalid char\n";
            return 0;
        }
    }

    source = workspaces::file_impl::replace_non_utf8_chars(source);

    try
    {
        auto s = antlrcpp::utf8_to_utf32((char*)data, (char*)(data + size));
    }
    catch (...)
    {
        // std::cout << "invalid UTF8\n";
        return 0;
    }

    // std::cout << "BEGINBEGIN\n" << source << "ENDEND\n";

    analyzer a(source);
    a.analyze();

    return 0; // Non-zero return values are reserved for future use.
}
