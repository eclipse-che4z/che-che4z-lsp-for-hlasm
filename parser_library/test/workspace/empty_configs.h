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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_EMPTY_CONFIGS_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_EMPTY_CONFIGS_H

#include <memory>
#include <string>
#include <string_view>

#include "nlohmann/json_fwd.hpp"
#include "utils/resource_location.h"

constexpr std::string_view pgm_conf_name(".hlasmplugin/pgm_conf.json");
constexpr std::string_view proc_grps_name(".hlasmplugin/proc_grps.json");
inline const hlasm_plugin::utils::resource::resource_location empty_ws("ews:/");
inline const hlasm_plugin::utils::resource::resource_location empty_pgm_conf_name("ews:/.hlasmplugin/pgm_conf.json");
inline const hlasm_plugin::utils::resource::resource_location empty_proc_grps_name("ews:/.hlasmplugin/proc_grps.json");
inline const std::string empty_pgm_conf = R"({ "pgms": []})";
inline const std::string empty_proc_grps = R"({ "pgroups": []})";

std::shared_ptr<const nlohmann::json> make_empty_shared_json();

#endif // !HLASMPLUGIN_PARSERLIBRARY_TEST_EMPTY_CONFIGS_H
