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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LIB_CONFIG_H
#define HLASMPLUGIN_PARSERLIBRARY_LIB_CONFIG_H

#include <memory>

#include "json.hpp"

#include "parser_library_export.h"

struct PARSER_LIBRARY_EXPORT lib_config
{
    static std::shared_ptr<lib_config> get_instance();
    static void load_from_json(const nlohmann::json& config);

    int64_t diag_supress_limit = 10;
};



#endif HLASMPLUGIN_PARSERLIBRARY_LIB_CONFIG_H
