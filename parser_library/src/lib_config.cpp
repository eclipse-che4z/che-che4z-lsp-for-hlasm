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

#include "lib_config.h"

std::shared_ptr<lib_config> lib_config::config_instance = std::make_shared<lib_config>();

std::shared_ptr<lib_config> lib_config::get_instance() { return std::atomic_load(&config_instance); }

void lib_config::load_from_json(const nlohmann::json& config)
{
    auto loaded = std::make_shared<lib_config>();

    auto found = config.find("diagnosticsSuppressLimit");
    if (found != config.end())
        loaded->diag_supress_limit = found->get<int64_t>();

    std::atomic_store(&config_instance, loaded);
}
