/*
 * Copyright (c) 2023 Broadcom.
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

#include "gmock/gmock.h"
#include "json_channel.h"

#include "nlohmann/json.hpp"

namespace {
struct mock_json_source : public hlasm_plugin::language_server::json_source
{
    MOCK_METHOD(std::optional<nlohmann::json>, read, (), (override));
};
struct mock_json_sink : public hlasm_plugin::language_server::json_sink
{
    MOCK_METHOD(void, write, (const nlohmann::json&), (override));
    MOCK_METHOD(void, write_rvr, (nlohmann::json &&));
    void write(nlohmann::json&& j) override { write_rvr(std::move(j)); }
};
struct mock_json_channel : public hlasm_plugin::language_server::json_channel
{
    MOCK_METHOD(std::optional<nlohmann::json>, read, (), (override));
    MOCK_METHOD(void, write, (const nlohmann::json&), (override));
    MOCK_METHOD(void, write_rvr, (nlohmann::json &&));
    void write(nlohmann::json&& j) override { write_rvr(std::move(j)); }
};
} // namespace
