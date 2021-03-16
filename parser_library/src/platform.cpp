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

#include "platform.h"

#ifdef __EMSCRIPTEN__
#    include <algorithm>
#    include <emscripten.h>

#    include <emscripten/bind.h>
#endif

namespace hlasm_plugin::parser_library::platform {
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

#ifdef __EMSCRIPTEN__
namespace {
std::string make_windows_preferred(std::string s)
{
    std::replace(s.begin(), s.end(), '/', '\\');
    return s;
}
std::string make_linux_preferred(std::string s)
{
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}
} // namespace
#endif

bool is_relative(const std::filesystem::path& p) { return !is_absolute(p); }
bool is_absolute(const std::filesystem::path& p)
{
#ifndef __EMSCRIPTEN__
    return p.is_absolute();
#else
    // emscripten implementation seems to be broken on windows
    if (is_windows())
    {
        const auto path = p.string();
        return (path.size() >= 2 && std::isalpha(path[0]) && path[1] == ':') // C:...
            || (path.size() >= 4 && path[0] == '\\' && path[1] == '\\'); // \\...\....;
    }
    else
        return p.is_absolute();
#endif
}

std::filesystem::path absolute_path(std::filesystem::path p)
{
    if (is_absolute(p))
        return p;

    return join_paths(std::filesystem::current_path(), p);
}

std::filesystem::path join_paths(const std::filesystem::path& left, const std::filesystem::path& right)
{
#ifndef __EMSCRIPTEN__
    return left / right;
#else
    // emscripten implementation seems to be broken on windows
    if (is_windows())
    {
        if (is_absolute(right))
            return right;
        return std::filesystem::path(make_windows_preferred((std::filesystem::path(make_linux_preferred(left.string()))
            / std::filesystem::path(make_linux_preferred(right.string())))
                                                                .string()));
    }
    else
        return left / right;
#endif
}

std::filesystem::path path_lexically_normal(const std::filesystem::path& p)
{
#ifndef __EMSCRIPTEN__
    return p.lexically_normal();
#else
    // emscripten implementation seems to be broken on windows
    if (is_windows())
    {
        auto p_ = make_linux_preferred(p.string());
        const bool unc = p_.size() >= 2 && p_[0] == '/' && p_[1] == '/';
        return std::filesystem::path(
            (unc ? "\\" : "") + make_windows_preferred(std::filesystem::path(p_).lexically_normal().string()));
    }
    else
        return p.lexically_normal();
#endif
}


std::filesystem::path path_lexically_relative(const std::filesystem::path& p, std::string q)
{
#ifndef __EMSCRIPTEN__
    return p.lexically_relative(q);
#else
    // emscripten implementation seems to be broken on windows
    if (is_windows())
        return std::filesystem::path(make_windows_preferred(std::filesystem::path(make_linux_preferred(p.string()))
                                                                .lexically_relative(make_linux_preferred(std::move(q)))
                                                                .string()));
    else
        return p.lexically_relative(q);
#endif
}

std::filesystem::path path_filename(const std::filesystem::path& p)
{
#ifndef __EMSCRIPTEN__
    return p.filename();
#else
    // emscripten implementation seems to be broken on windows
    if (is_windows())
        return std::filesystem::path(make_linux_preferred(p.string())).filename();
    else
        return p.filename();
#endif
}

bool path_equal(const std::filesystem::path& left, const std::filesystem::path& right)
{
#ifndef __EMSCRIPTEN__
    return left == right;
#else
    // emscripten implementation seems to be broken on windows
    if (is_windows())
        return std::filesystem::path(make_linux_preferred(left.string()))
            == std::filesystem::path(make_linux_preferred(right.string()));
    else
        return left == right;
#endif
}


#ifdef __EMSCRIPTEN__

class directory_listing
{
    std::string buffer;
    std::function<void(const std::filesystem::path&)> handler;

public:
    directory_listing(std::function<void(const std::filesystem::path&)> h)
        : handler(h)
    {}

    list_directory_rc run(const std::filesystem::path& d)
    {
        auto path = absolute_path(d).string();

        int result = MAIN_THREAD_EM_ASM_INT(
            {
                let rc = 0;
                try
                {
                    const dir_name = UTF8ToString($0);
                    const fs = require('fs');
                    const path = require('path');

                    fs.accessSync(dir_name);

                    rc = 1;

                    const dir = fs.opendirSync(dir_name);

                    rc = 2;

                    let de = null;
                    while (de = dir.readSync())
                    {
                        if (de.isFile())
                        {
                            const file_name = path.join(dir_name, de.name);
                            const buf_len = lengthBytesUTF8(file_name);
                            const ptr = Module.directory_listing_get_buffer($1, buf_len);
                            stringToUTF8(file_name, ptr, buf_len + 1);
                            Module.directory_listing_commit_buffer($1);
                        }
                    }
                    rc = 3;
                }
                catch (e)
                {}
                return rc;
            },
            (intptr_t)path.c_str(),
            (intptr_t)this);

        switch (result)
        {
            case 0:
                return list_directory_rc::not_exists;
            case 1:
                return list_directory_rc::not_a_directory;
            case 2:
                return list_directory_rc::other_failure;
            case 3:
                return list_directory_rc::done;
            default:
                throw std::logic_error("unreachable");
        }
    }

    static intptr_t get_buffer(intptr_t this_, int size)
    {
        auto ptr = reinterpret_cast<directory_listing*>(this_);
        ptr->buffer.resize(size);
        return reinterpret_cast<intptr_t>(ptr->buffer.data());
    }

    static void commit_buffer(intptr_t this_)
    {
        auto ptr = reinterpret_cast<directory_listing*>(this_);
        ptr->handler(ptr->buffer);
    }
};

EMSCRIPTEN_BINDINGS(directory_listing_function)
{
    emscripten::function("directory_listing_get_buffer", &directory_listing::get_buffer);
    emscripten::function("directory_listing_commit_buffer", &directory_listing::commit_buffer);
}
#endif

list_directory_rc list_directory_regular_files(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
#ifndef __EMSCRIPTEN__
    std::filesystem::directory_entry dir(d);

    if (!dir.exists())
        return list_directory_rc::not_exists;

    if (!dir.is_directory())
        return list_directory_rc::not_a_directory;

    try
    {
        std::filesystem::directory_iterator it(dir);

        for (auto& p : it)
        {
            if (p.is_regular_file())
            {
                h(p.path());
            }
        }
    }
    catch (const std::filesystem::filesystem_error&)
    {
        return list_directory_rc::other_failure;
    }

    return list_directory_rc::done;
#else
    // directory listing seems broken every where
    directory_listing l(h);
    return l.run(d);
#endif
}

} // namespace hlasm_plugin::parser_library::platform