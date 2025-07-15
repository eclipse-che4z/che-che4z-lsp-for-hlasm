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

#include "gmock/gmock.h"

#include "feature.h"

namespace hlasm_plugin::language_server {

class response_provider_mock : public response_provider
{
public:
    MOCK_METHOD(void,
        request,
        (std::string_view requested_method,
            nlohmann::json&& args,
            std::function<void(const nlohmann::json& params)> handler,
            std::function<void(int, const char*)> error_handler),
        (override));
    MOCK_METHOD(
        void, respond, (const request_id& id, std::string_view requested_method, nlohmann::json&& args), (override));
    MOCK_METHOD(void, notify, (std::string_view method, nlohmann::json&& args), (override));
    MOCK_METHOD(void,
        respond_error,
        (const request_id& id,
            std::string_view requested_method,
            int err_code,
            std::string_view err_message,
            nlohmann::json&& error),
        (override));
    MOCK_METHOD(void, register_cancellable_request, (const request_id&, request_invalidator), (override));
};

} // namespace hlasm_plugin::language_server
