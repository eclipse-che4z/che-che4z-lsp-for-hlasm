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
#include <iostream>

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
    if (is_windows())
    {
        const auto path = p.string();
        return (path.size() >= 2 && std::isalpha((unsigned char)path[0]) && path[1] == ':') // C:...
            || (path.size() >= 4 && path[0] == '\\' && path[1] == '\\'); // \\...\....;
    }
    else
        return p.is_absolute();
}

std::filesystem::path absolute(const std::filesystem::path& p)
{
    if (p.empty() || is_absolute(p))
        return p;

    return join(std::filesystem::current_path(), p);
}

std::filesystem::path join(const std::filesystem::path& left, const std::filesystem::path& right)
{
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
}

std::filesystem::path lexically_normal(const std::filesystem::path& p)
{
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
}


std::filesystem::path lexically_relative(const std::filesystem::path& p, std::string q)
{
    // emscripten implementation seems to be broken on windows
    if (is_windows())
        return std::filesystem::path(make_windows_preferred(std::filesystem::path(make_linux_preferred(p.string()))
                                                                .lexically_relative(make_linux_preferred(std::move(q)))
                                                                .string()));
    else
        return p.lexically_relative(q);
}

std::filesystem::path filename(const std::filesystem::path& p)
{
    // emscripten implementation seems to be broken on windows
    if (is_windows())
        return std::filesystem::path(make_linux_preferred(p.string())).filename();
    else
        return p.filename();
}

bool equal(const std::filesystem::path& left, const std::filesystem::path& right)
{
    // emscripten implementation seems to be broken on windows
    if (is_windows())
        return std::filesystem::path(make_linux_preferred(left.string()))
            == std::filesystem::path(make_linux_preferred(right.string()));
    else
        return left == right;
}

class directory_listing
{
    std::string buffer;
    std::function<void(const std::filesystem::path&)> handler;

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

public:
    directory_listing(std::function<void(const std::filesystem::path&)> h)
        : handler(h)
    {
        static thread_local bool registered = false;
        if (!registered)
        {
            registered = true;
            emscripten::function("directory_listing_get_buffer", &directory_listing::get_buffer);
            emscripten::function("directory_listing_commit_buffer", &directory_listing::commit_buffer);
        }
    }

    list_directory_rc run(const std::filesystem::path& d)
    {
        auto path = path::absolute(d).string();

        int result = EM_ASM_INT(
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
    list_directory_rc get_libraries_current(const std::filesystem::path& d)
    {
        auto path = path::absolute(d).string();

        int result = EM_ASM_INT(
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
                        if (de.isDirectory())
                        {
                            const file_name = path.join(dir_name, de.name);
                            const buf_len = lengthBytesUTF8(file_name);
                            const ptr = Module.directory_listing_get_buffer($1, buf_len);
                            stringToUTF8(file_name, ptr, buf_len + 1);
                            Module.directory_listing_commit_buffer($1);
                        }
                        if (de.isSymbolicLink())
                        {
                            // resolve symlink to actual existing path
                            const file_name = fs.realpathSync(path.join(dir_name, de.name));
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
    list_directory_rc get_libraries_recursive(const std::filesystem::path& d, const std::string suffix_path)
    {
        auto path = path::absolute(d).lexically_normal().string();
        int result = EM_ASM_INT(
            {
                let rc = 0;
                try
                {
                    const dir_name = UTF8ToString($0);
                    const suffix_path = UTF8ToString($2);
                    const recursion_limit = 99;
                    const fs = require('fs');
                    const path = require('path');

                    fs.accessSync(dir_name);

                    rc = 1;

                    const access = fs.opendirSync(dir_name);

                    rc = 2;

                    const getFilesRecursively = (directory, recursion_depth) =>
                    {
                        if (recursion_depth > recursion_limit)
                            return;
                        let de = null;
                        const dir = fs.opendirSync(directory);
                        while (de = dir.readSync())
                        {
                            const file_name = path.join(directory, de.name);
                            if (de.isSymbolicLink())
                            {
                                
                                if (file_name.match(suffix_path))
                                {
                                    const resolved_link = fs.realpathSync(file_name);
                                    const buf_len = lengthBytesUTF8(resolved_link);
                                    const ptr = Module.directory_listing_get_buffer($1, buf_len);
                                    stringToUTF8(resolved_link, ptr, buf_len + 1);
                                    Module.directory_listing_commit_buffer($1);
                                }
                                getFilesRecursively(file_name, recursion_depth++);
                            }
                            if (de.isDirectory() )
                            {   
                                if (file_name.match(suffix_path))
                                {
                                    const buf_len = lengthBytesUTF8(file_name);
                                    const ptr = Module.directory_listing_get_buffer($1, buf_len);
                                    stringToUTF8(file_name, ptr, buf_len + 1);
                                    Module.directory_listing_commit_buffer($1);
                                }
                                getFilesRecursively(file_name, recursion_depth++);
                            }
                        }
                    };
                    getFilesRecursively(dir_name, 1);
                    rc = 3;
                }
                catch (e)
                {}
                return rc;
            },
            (intptr_t)path.c_str(),
            (intptr_t)this,
            (intptr_t)suffix_path.c_str());

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
}; // namespace hlasm_plugin::utils::path
list_directory_rc list_directory_regular_files(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
    // directory listing seems broken everywhere
    directory_listing l(h);
    return l.run(d);
}
list_directory_rc list_current_directory(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
    // directory listing seems broken everywhere
    directory_listing l(h);
    return l.get_libraries_current(d);
}
list_directory_rc list_directory_recursively(
    const std::filesystem::path& d, const std::string& suffix_path, std::function<void(const std::filesystem::path&)> h)
{
    directory_listing l(h);
    return l.get_libraries_recursive(d, suffix_path);
}
} // namespace hlasm_plugin::utils::path