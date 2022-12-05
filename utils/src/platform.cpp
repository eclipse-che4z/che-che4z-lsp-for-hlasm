/*
 * Copyright (c) 2021 Broadcom.
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

#include "utils/platform.h"

#ifdef __EMSCRIPTEN__
#    include <emscripten.h>

#    include <emscripten/bind.h>
#else
#    include <fstream>
#endif

namespace hlasm_plugin::utils::platform {

bool is_windows()
{
#ifdef _WIN32
    return true;
#elif __EMSCRIPTEN__
    // clang-format off
    static const bool windows_flag = []() { return EM_ASM_INT({ return process.platform === "win32" ? 1 : 0; }); }();
    // clang-format on
    return windows_flag;
#else
    return false;
#endif
}

const std::string& home()
{
#ifdef _WIN32
    static const std::string home_dir = []() {
        std::string result;
        constexpr auto get_env = [](const char* c) -> std::optional<std::string> {
            if (auto* r = std::getenv(c))
                return std::string(r);
            return std::nullopt;
        };
        auto userprofile = get_env("USERPROFILE");
        auto homedrive = get_env("HOMEDRIVE");
        auto homepath = get_env("HOMEPATH");
        auto home = get_env("HOME");

        if (userprofile)
            result = std::move(userprofile.value());
        else if (homedrive && homepath)
            result = std::move(homedrive.value()) + std::move(homepath.value());
        else if (home)
            result = std::move(home.value());

        if (!result.empty())
            result.front() = std::tolower((unsigned char)result.front());

        return result;
    }();
#elif __EMSCRIPTEN__
    static const std::string home_dir = []() {
        std::string s;

        auto resize = +[](intptr_t ptr, ssize_t size) {
            auto str = (std::string*)ptr;
            str->resize(size);
            return (intptr_t)str->data();
        };
        emscripten::function("resize_string", resize);
        // clang-format off
        EM_ASM(
            {
                const homedir = require('os').homedir();
                if (homedir)
                {
                    const len = lengthBytesUTF8(homedir);
                    const ptr = Module.resize_string($0, len);
                    stringToUTF8(homedir, ptr, len + 1);
                }
            },
            (intptr_t)&s);
        // clang-format on

        if (!s.empty() && is_windows())
            s.front() = std::tolower((unsigned char)s.front());

        return s;
    }();
#else
    static const std::string home_dir = []() {
        if (const char* home = std::getenv("HOME"); home)
            return std::string(home);

        return std::string();
    }();
#endif
    return home_dir;
}


std::optional<std::string> read_file(const std::string& file)
{
#if __EMSCRIPTEN__
    std::optional<std::string> s;

    [[maybe_unused]] thread_local const bool prepare_buffer_registered = []() {
        auto prepare_buffer = +[](intptr_t ptr, ssize_t size) {
            auto& str = *(std::optional<std::string>*)ptr;
            str.emplace((size_t)size, '\0');
            return (intptr_t)str->data();
        };
        emscripten::function("read_file_prepare_buffer", prepare_buffer);
        return true;
    }();
    // clang-format off
        EM_ASM(
            {
                try
                {
                    const content = require('fs').readFileSync(UTF8ToString($1));
                    const ptr = Module.read_file_prepare_buffer($0, content.length);
                    content.copy(new Uint8Array(Module.HEAPU8.buffer, ptr, content.length));
                }
                catch (e) {}
            },
            (intptr_t)&s, (intptr_t)file.c_str());
    // clang-format on

    return s;
#else
    std::ifstream fin(file, std::ios::in | std::ios::binary);

    if (fin)
    {
        try
        {
            fin.seekg(0, std::ios::end);
            auto file_size = fin.tellg();

            if (file_size == -1)
                return std::nullopt;

            std::optional<std::string> text(std::in_place, (size_t)file_size, '\0');
            fin.seekg(0, std::ios::beg);
            fin.read(text->data(), text->size());
            fin.close();

            return text;
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }
    else
    {
        return std::nullopt;
    }
#endif
}

} // namespace hlasm_plugin::utils::platform
