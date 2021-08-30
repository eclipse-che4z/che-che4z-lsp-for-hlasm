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

#include "wildcard.h"

#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>

#include "utils/path.h"
#include "utils/platform.h"
using list_directory_result =
    std::pair<std::unordered_map<std::string, std::string>, hlasm_plugin::utils::path::list_directory_rc>;
namespace hlasm_plugin::parser_library::workspaces {
namespace {
// used for wildcard to regex conversions
const std::regex escape("(\\(|\\[|\\{|\\\\|\\^|\\-|\\=|\\$|\\!|\\||\\]|\\}|\\)|\\.)");
const std::regex question("\\?");
const std::regex nongreedy("(\\*|\\+)");
const std::regex slash("\\/");
} // namespace

std::regex wildcard2regex(const std::string& wildcard)
{
    auto regex_str = wildcard;
    if (utils::platform::is_windows())
    {
        // change of forward slash to double backslash on windows
        regex_str = std::regex_replace(regex_str, slash, "\\");
    }
    regex_str = std::regex_replace(regex_str, escape, "\\$1");
    regex_str = std::regex_replace(regex_str, question, ".");
    regex_str = std::regex_replace(regex_str, nongreedy, ".$1?");
    return std::regex(regex_str);
}
/*std::set<std::filesystem::path> wildcard_recursive_search(std::string lib_path, int pos)
{
    std::set<std::filesystem::path> list_of_libs;

    auto prefix_path = lib_path.substr(0, pos);
    pos = pos + 1;
    for (const auto& p : std::filesystem::recursive_directory_iterator(prefix_path))
    {
        std::string suffix_path = lib_path.substr(pos);
        suffix_path.pop_back();
        suffix_path = std::regex_replace(suffix_path, std::regex("\\*"), ".*");
        std::string directory_path = p.path().lexically_normal().generic_string();
        std::regex fileMatcher(
            prefix_path + suffix_path, std::regex_constants::ECMAScript | std::regex_constants::icase);
        if (p.is_directory() && std::regex_search(directory_path, fileMatcher))

        {
            list_of_libs.insert(p.path().lexically_normal());
        }
    }
    return list_of_libs;
}*/
std::set<std::filesystem::path> wildcard_current_search(std::string lib_path, int pos)
{
    list_directory_result result;
    std::set<std::filesystem::path> list_of_libs;
    auto substr = std::filesystem::path(lib_path.substr(0, pos));

    pos = pos + 2;
    result.second = utils::path::list_current_directory(
        substr, [&result, &list_of_libs, &lib_path, &pos](const std::filesystem::path& f) {
            result.first[utils::path::filename(f).string()] = utils::path::absolute(f).string();
            list_of_libs.insert(
                utils::path::absolute(f).append(lib_path.substr(pos, lib_path.size())).lexically_normal());
        });


    return list_of_libs;
}
std::set<std::filesystem::path> wildcard_recursive_search(std::string lib_path, int pos)
{
    list_directory_result result;
    std::set<std::filesystem::path> list_of_libs;
    auto substr = std::filesystem::path(lib_path.substr(0, pos));
    pos = pos + 1;
    std::string suffix_path = lib_path.substr(pos);
    suffix_path.pop_back();
    suffix_path = std::regex_replace(suffix_path, std::regex("\\*"), ".*");

    result.second = utils::path::list_directory_recursively(
        substr, suffix_path, [&result, &list_of_libs](const std::filesystem::path& f) {
            result.first[utils::path::filename(f).string()] = utils::path::absolute(f).string();
            list_of_libs.insert(utils::path::absolute(f).lexically_normal());
        });


    return list_of_libs;
}
} // namespace hlasm_plugin::parser_library::workspaces