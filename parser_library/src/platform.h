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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PLATFORM_H
#define HLASMPLUGIN_PARSERLIBRARY_PLATFORM_H

#include <filesystem>
#include <functional>
#include <string>

namespace hlasm_plugin::parser_library::platform {
bool is_windows();

bool is_relative(const std::filesystem::path&);
bool is_absolute(const std::filesystem::path&);

std::filesystem::path absolute_path(std::filesystem::path p);
std::filesystem::path join_paths(const std::filesystem::path& left, const std::filesystem::path& right);
std::filesystem::path path_lexically_normal(const std::filesystem::path& p);
std::filesystem::path path_lexically_relative(const std::filesystem::path& p, std::string q);
std::filesystem::path path_filename(const std::filesystem::path& p);
bool path_equal(const std::filesystem::path& left, const std::filesystem::path& right);

enum class list_directory_rc
{
    done,
    not_exists,
    not_a_directory,
    other_failure,
};

list_directory_rc list_directory_regular_files(
    const std::filesystem::path& d, std::function<void(const std::filesystem::path&)> h);
} // namespace hlasm_plugin::parser_library::platform

#endif
