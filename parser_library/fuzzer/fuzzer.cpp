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

#include <array>
#include <bitset>
#include <cassert>
#include <charconv>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "analyzer.h"
#include "analyzing_context.h"
#include "preprocessor_options.h"
#include "utils/resource_location.h"
#include "utils/unicode_text.h"
#include "workspaces/file_impl.h"
#include "workspaces/parse_lib_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;

class fuzzer_lib_provider : public parse_lib_provider
{
    std::optional<size_t> read_library_name(std::string_view library) const
    {
        if (library.size() < 2 || library.size() > 8 || library[0] != '@'
            || std::any_of(library.begin() + 1, library.end(), [](unsigned char c) { return !isdigit(c); }))
            return std::nullopt;

        size_t result;
        std::from_chars(library.data() + 1, library.data() + library.size(), result);
        if (result >= files.size())
            return std::nullopt;

        return result;
    }

public:
    void parse_library(
        std::string_view library, analyzing_context ctx, library_data data, std::function<void(bool)> callback) override
    {
        assert(callback);
        auto lib = read_library_name(library);
        if (!lib.has_value())
        {
            callback(false);
            return;
        }

        auto a = std::make_unique<analyzer>(files[lib.value()],
            analyzer_options { hlasm_plugin::utils::resource::resource_location(library), this, std::move(ctx), data });
        a->analyze();
        a->collect_diags();
        callback(true);
    }

    bool has_library(std::string_view library) const override { return read_library_name(library).has_value(); }

    void get_library(std::string_view library,
        std::function<void(std::optional<std::pair<std::string, utils::resource::resource_location>>)> callback)
        const override
    {
        assert(callback);
        auto lib = read_library_name(library);
        if (!lib.has_value())
            return callback(std::nullopt);

        return callback(std::pair<std::string, hlasm_plugin::utils::resource::resource_location>(
            files[lib.value()], hlasm_plugin::utils::resource::resource_location(library)));
    }

    std::vector<std::string> files;
};

namespace {
static const std::array<preprocessor_options, 3> preproc_options = {
    endevor_preprocessor_options(),
    cics_preprocessor_options(),
    db2_preprocessor_options(),
};

std::vector<preprocessor_options> get_preprocessor_options(std::bitset<3> b)
{
    static_assert(b.size() == preproc_options.size());

    std::vector<preprocessor_options> opts;
    for (size_t i = 0; i < preproc_options.size(); ++i)
    {
        if (b[i])
            opts.emplace_back(preproc_options[i]);
    }

    return opts;
}

std::string get_content(const uint8_t* data, size_t size, fuzzer_lib_provider& lib)
{
    std::string source;
    std::string* target = &source;

    while (auto next = (const uint8_t*)memchr(data, 0xff, size))
    {
        *target = hlasm_plugin::utils::replace_non_utf8_chars(std::string_view((const char*)data, next - data));

        target = &lib.files.emplace_back();
        size -= next + 1 - data;
        data = next + 1;
    }
    *target = hlasm_plugin::utils::replace_non_utf8_chars(std::string_view((const char*)data, size));

    return source;
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (!data || size <= 1)
        return 0;

    fuzzer_lib_provider lib;
    auto source = get_content(data + 1, size - 1, lib);
    analyzer a(source, analyzer_options(&lib, get_preprocessor_options(std::bitset<3>(data[0]))));
    a.analyze();

    return 0; // Non-zero return values are reserved for future use.
}
