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
using utils::platform::is_web;
using utils::platform::is_windows;

namespace {
std::string make_windows_preferred(std::string s)
{
    std::ranges::replace(s, '/', '\\');
    return s;
}
std::string make_linux_preferred(std::string s)
{
    std::ranges::replace(s, '\\', '/');
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

    return join(current_path(), p);
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

std::filesystem::path parent_path(const std::filesystem::path& p)
{
    // emscripten implementation seems to be broken on windows
    if (is_windows())
        return std::filesystem::path(make_linux_preferred(p.string())).parent_path();
    else
        return p.parent_path();
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
        static thread_local bool registered = EM_ASM_INT(
            { return !!Module.directory_op_support_get_buffer && !!Module.directory_op_support_commit_buffer; });
        if (!registered)
        {
            registered = true;
            emscripten::function("directory_op_support_get_buffer", &directory_op_support::get_buffer);
            emscripten::function("directory_op_support_commit_buffer", &directory_op_support::commit_buffer);
        }
    }

    list_directory_rc files(const std::filesystem::path& d)
    {
        auto path = path::absolute(d).string();

        int result = EM_ASM_INT(
            {
                let rc = 0;
                let dir = null;
                try
                {
                    const dir_name = UTF8ToString($0);
                    const fs = require('fs');
                    const path = require('path');

                    fs.accessSync(dir_name);

                    rc = 1;

                    dir = fs.opendirSync(dir_name);

                    rc = 2;

                    let de = null;
                    while (de = dir.readSync())
                    {
                        if (de.isFile())
                        {
                            const file_name = path.join(dir_name, de.name);
                            const buf_len = lengthBytesUTF8(file_name);
                            const ptr = Module.directory_op_support_get_buffer($1, buf_len);
                            stringToUTF8(file_name, ptr, buf_len + 1);
                            Module.directory_op_support_commit_buffer($1);
                        }
                    }
                    rc = 3;
                }
                catch (e)
                {}
                finally
                {
                    if (dir)
                        dir.closeSync();
                }
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

    list_directory_rc subdirs_and_symlinks(const std::filesystem::path& d)
    {
        auto path = path::absolute(d).string();

        int result = EM_ASM_INT(
            {
                let rc = 0;
                let dir = null;
                try
                {
                    const dir_name = UTF8ToString($0);
                    const fs = require('fs');
                    const path = require('path');

                    fs.accessSync(dir_name);

                    rc = 1;

                    dir = fs.opendirSync(dir_name);

                    rc = 2;

                    let de = null;
                    while (de = dir.readSync())
                    {
                        if (de.isDirectory() || de.isSymbolicLink())
                        {
                            const file_name = path.join(dir_name, de.name);
                            const buf_len = lengthBytesUTF8(file_name);
                            const ptr = Module.directory_op_support_get_buffer($1, buf_len);
                            stringToUTF8(file_name, ptr, buf_len + 1);
                            Module.directory_op_support_commit_buffer($1);
                        }
                    }
                    rc = 3;
                }
                catch (e)
                {}
                finally
                {
                    if (dir)
                        dir.closeSync();
                }
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

    std::filesystem::path realpath(const std::filesystem::path& path, std::error_code& ec)
    {
        buffer.clear();
        int result = EM_ASM_INT(
            {
                try
                {
                    const real_name = require('fs').realpathSync(UTF8ToString($0));

                    const buf_len = lengthBytesUTF8(real_name);
                    const ptr = Module.directory_op_support_get_buffer($1, buf_len);
                    stringToUTF8(real_name, ptr, buf_len + 1);
                    Module.directory_op_support_commit_buffer($1);

                    return 0;
                }
                catch (e)
                {
                    return e.errno || -1;
                }
            },
            (intptr_t)path.c_str(),
            (intptr_t)this);

        if (result != 0)
            ec = std::error_code(-result, std::system_category());

        return buffer;
    }

    std::filesystem::path cwd(std::error_code& ec)
    {
        buffer.clear();
        if (is_web())
            return buffer;

        int result = EM_ASM_INT(
            {
                try
                {
                    const cwd = process.cwd();

                    const buf_len = lengthBytesUTF8(cwd);
                    const ptr = Module.directory_op_support_get_buffer($0, buf_len);
                    stringToUTF8(cwd, ptr, buf_len + 1);
                    Module.directory_op_support_commit_buffer($0);

                    return 0;
                }
                catch (e)
                {
                    return e.errno || -1;
                }
            },
            (intptr_t)this);

        if (result != 0)
            ec = std::error_code(-result, std::system_category());

        return buffer;
    }

    bool is_dir(const std::filesystem::path& path)
    {
        int result = EM_ASM_INT(
            {
                try
                {
                    if (require('fs').statSync(UTF8ToString($0)).isDirectory())
                        return 1;
                    else
                        return 0;
                }
                catch (e)
                {
                    return -1;
                }
            },
            (intptr_t)path.c_str());

        return result == 1;
    }
};

std::filesystem::path current_path()
{
    // cwd listing seems broken on windows
    std::error_code ec;
    auto result = directory_op_support().cwd(ec);

    if (ec)
        throw std::filesystem::filesystem_error("current_path", ec);

    return result;
}

list_directory_rc list_directory_regular_files(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
    // directory listing seems broken everywhere
    directory_op_support l(h);
    return l.files(d);
}


list_directory_rc list_directory_subdirs_and_symlinks(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
    // directory listing seems broken everywhere
    directory_op_support l(h);
    return l.subdirs_and_symlinks(d);
}

std::filesystem::path canonical(const std::filesystem::path& p)
{
    std::error_code ec;
    auto result = directory_op_support().realpath(p, ec);

    if (ec)
        throw std::filesystem::filesystem_error("realpath error", p.string(), ec);

    return result;
}
std::filesystem::path canonical(const std::filesystem::path& p, std::error_code& ec)
{
    return directory_op_support().realpath(p, ec);
}


bool is_directory(const std::filesystem::path& p) { return directory_op_support().is_dir(p); }

} // namespace hlasm_plugin::utils::path
