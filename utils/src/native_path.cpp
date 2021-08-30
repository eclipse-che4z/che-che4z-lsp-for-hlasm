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
#include <iostream>
#include <regex>

#include "utils/path.h"
#include "utils/platform.h"

namespace hlasm_plugin::utils::path {
const int RECURSION_LIMIT = 99;
bool is_relative(const std::filesystem::path& p) { return !is_absolute(p); }
bool is_absolute(const std::filesystem::path& p) { return p.is_absolute(); }

std::filesystem::path absolute(const std::filesystem::path& p) { return std::filesystem::absolute(p); }

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

bool equal(const std::filesystem::path& left, const std::filesystem::path& right) { return left == right; }

list_directory_rc list_directory_regular_files(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
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
}
list_directory_rc list_current_directory(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h)
{
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
            if (p.is_directory())
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
list_directory_rc list_directory_recursively(const std::filesystem::path& d,
    const std::string& suffix_string,
    std::function<void(const std::filesystem::path&)> h)
{
    std::filesystem::directory_entry dir(d);

    if (!dir.exists())
        return list_directory_rc::not_exists;

    if (!dir.is_directory())
        return list_directory_rc::not_a_directory;

    try
    {
        std::filesystem::recursive_directory_iterator it(dir);

        for (auto& p : it)
        {
            if (it.depth() >= RECURSION_LIMIT)
            {
                it.disable_recursion_pending();
                continue;
            }
            if (p.is_symlink())
            {
                h(std::filesystem ::canonical(p));
            }
            std::string directory_path = p.path().lexically_normal().generic_string();
            std::regex fileMatcher(
                d.string() + suffix_string, std::regex_constants::ECMAScript | std::regex_constants::icase);
            if (p.is_directory() && std::regex_search(directory_path, fileMatcher))
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
} // namespace hlasm_plugin::utils::path
