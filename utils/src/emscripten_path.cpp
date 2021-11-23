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
#include <algorithm>
#include <emscripten.h>

#include <emscripten/bind.h>

#include "utils/path.h"
#include "utils/platform.h"

namespace hlasm_plugin::utils::path {
using utils::platform::is_windows;

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

bool is_relative(const std::filesystem::path& p) { return !is_absolute(p); }
bool is_absolute(const std::filesystem::path& p)
{
    // emscripten implementation seems to be broken on windows
    /* if (is_windows())
    {
        const auto path = p.string();
        return (path.size() >= 2 && std::isalpha((unsigned char)path[0]) && path[1] == ':') // C:...
            || (path.size() >= 4 && path[0] == '\\' && path[1] == '\\'); // \\...\....;
    }
    else
        return p.is_absolute();*/
    return false;
}

std::filesystem::path absolute(const std::filesystem::path& p)
{
    if (p.empty() || is_absolute(p))
        return p;

    return p;
}

std::filesystem::path join(const std::filesystem::path& left, const std::filesystem::path& right)
{
    // emscripten implementation seems to be broken on windows
    if (is_windows())
    {
        return left;
    }
    else
        return left / right;
}

std::filesystem::path lexically_normal(const std::filesystem::path& p)
{
    // emscripten implementation seems to be broken on windows
    if (is_windows())
    {
        return p;
    }
    else
        return p.lexically_normal();
}


std::filesystem::path lexically_relative(const std::filesystem::path& p, std::string q)
{
    // emscripten implementation seems to be broken on windows
    if (is_windows())
        return p;
    else
        return p.lexically_relative(q);
}

std::filesystem::path filename(const std::filesystem::path& p)
{
    // emscripten implementation seems to be broken on windows
    if (is_windows())
        return p;
    else
        return p.filename();
}

bool equal(const std::filesystem::path& left, const std::filesystem::path& right)
{
    // emscripten implementation seems to be broken on windows
    return true;
}

class directory_op_support
{
    std::string buffer;
    std::function<void(const std::filesystem::path&)> handler;

    static intptr_t get_buffer(intptr_t this_, int size)
    {
        auto ptr = reinterpret_cast<directory_op_support*>(this_);
        ptr->buffer.resize(size);
        return reinterpret_cast<intptr_t>(ptr->buffer.data());
    }

    static void commit_buffer(intptr_t this_)
    {
        auto ptr = reinterpret_cast<directory_op_support*>(this_);
        if (ptr->handler)
            ptr->handler(ptr->buffer);
    }

public:
    directory_op_support(std::function<void(const std::filesystem::path&)> h = {})
        : handler(h)
    {
        static thread_local bool registered = false;
        if (!registered)
        {
            registered = true;
        }
    }

    list_directory_rc files(const std::filesystem::path& d)
    {
     return list_directory_rc::done;
    }

    list_directory_rc subdirs_and_symlinks(const std::filesystem::path& d)
    {
        return list_directory_rc::done;
    }

    std::filesystem::path realpath(const std::filesystem::path& path, std::error_code& ec)
    {
        

        return buffer;
    }

    bool is_dir(const std::filesystem::path& path)
    {
        return false;
    }
};

list_directory_rc list_directory_regular_files(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
    return list_directory_rc::done;
}


list_directory_rc list_directory_subdirs_and_symlinks(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
    return list_directory_rc::done;
}

std::filesystem::path canonical(const std::filesystem::path& p)
{ return p; }
std::filesystem::path canonical(const std::filesystem::path& p, std::error_code& ec)
{
    return p;
}


bool is_directory(const std::filesystem::path& p) { return false; }

} // namespace hlasm_plugin::utils::path
