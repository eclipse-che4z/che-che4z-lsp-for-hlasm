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
#include "utils/path.h"
#include "utils/platform.h"

namespace hlasm_plugin::utils::path {

bool is_relative(const std::filesystem::path& p) { return !is_absolute(p); }
bool is_absolute(const std::filesystem::path& p) { return p.is_absolute(); }

std::filesystem::path absolute(const std::filesystem::path& p) { return std::filesystem::absolute(p); }

std::filesystem::path current_path() { return std::filesystem::current_path(); }

std::filesystem::path join(const std::filesystem::path& left, const std::filesystem::path& right)
{
    return left / right;
}

std::filesystem::path lexically_normal(const std::filesystem::path& p) { return p.lexically_normal(); }

std::filesystem::path lexically_relative(const std::filesystem::path& p, std::string q)
{
    return p.lexically_relative(q);
}

std::filesystem::path filename(const std::filesystem::path& p) { return p.filename(); }
std::filesystem::path parent_path(const std::filesystem::path& p) { return p.parent_path(); }

std::filesystem::path canonical(const std::filesystem::path& p) { return std::filesystem::canonical(p); }
std::filesystem::path canonical(const std::filesystem::path& p, std::error_code& ec)
{
    return std::filesystem::canonical(p, ec);
}

bool equal(const std::filesystem::path& left, const std::filesystem::path& right) { return left == right; }
bool is_directory(const std::filesystem::path& p)
{
    std::error_code ec;
    std::filesystem::directory_entry d(p, ec);

    return !ec && d.is_directory();
}

list_directory_rc list_directory_regular_files(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
    try
    {
        std::filesystem::directory_entry dir(d);

        if (!dir.exists())
            return list_directory_rc::not_exists;

        if (!dir.is_directory())
            return list_directory_rc::not_a_directory;

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
}

list_directory_rc list_directory_subdirs_and_symlinks(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
    try
    {
        std::filesystem::directory_entry dir(d);

        if (!dir.exists())
            return list_directory_rc::not_exists;

        if (!dir.is_directory())
            return list_directory_rc::not_a_directory;

        std::filesystem::directory_iterator it(dir);

        for (auto& p : it)
        {
            auto path = p.path();
            if (std::filesystem::directory_entry de(path); de.is_directory() || de.is_symlink())
                h(path);
        }
    }
    catch (const std::filesystem::filesystem_error&)
    {
        return list_directory_rc::other_failure;
    }

    return list_directory_rc::done;
}

} // namespace hlasm_plugin::utils::path
