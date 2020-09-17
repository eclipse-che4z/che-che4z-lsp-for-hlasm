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

#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_H

#include <map>
#include <string>

#include "json.hpp"

#include "common_types.h"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server {

// Provides methods to send notification, respond to request and respond with error respond
class response_provider
{
public:
    virtual void request(const json& id, const std::string& requested_method, const json& args) = 0;
    virtual void respond(const json& id, const std::string& requested_method, const json& args) = 0;
    virtual void notify(const std::string& method, const json& args) = 0;
    virtual void respond_error(const json& id,
        const std::string& requested_method,
        int err_code,
        const std::string& err_message,
        const json& error) = 0;
    virtual ~response_provider() = default;
};

// Abstract class for group of methods that add functionality to server.
class feature
{
public:
    // Constructs the feature with workspace_manager.
    // All the requests and notification are passed to the workspace manager
    explicit feature(parser_library::workspace_manager& ws_mngr)
        : ws_mngr_(ws_mngr)
    {}
    // Constructs the feature with workspace_manager and response_provider through which the feature can send messages.
    feature(parser_library::workspace_manager& ws_mngr, response_provider& response_provider)
        : ws_mngr_(ws_mngr)
        , response_(&response_provider)
    {}

    // Implement to add methods to server.
    void virtual register_methods(std::map<std::string, method>& methods) = 0;
    // Implement to add json object to server capabilities that are sent to LSP client
    // in the response to initialize request.
    json virtual register_capabilities() = 0;

    // Can be implemented to set feature specific to client capabilities that
    // are sent in initialize request.
    void virtual initialize_feature(const json& client_capabilities) = 0;

    // Converts URI (RFC3986) to common filesystem path.
    static std::string uri_to_path(const std::string& uri);
    // Converts from filesystem path to URI
    static std::string path_to_uri(std::string path);

    // Converts LSP json representation of range into parse_library::range.
    static parser_library::range parse_range(const json& range_json);
    // Converts LSP json representation of position into parse_library::position.
    static parser_library::position parse_position(const json& position_json);

    static json range_to_json(const parser_library::range& range);
    static json position_to_json(const parser_library::position& position);

    virtual ~feature() = default;

protected:
    parser_library::workspace_manager& ws_mngr_;
    bool callbacks_registered_ = false;
    response_provider* response_ = nullptr;
};

} // namespace hlasm_plugin::language_server

#endif // !HLASMPLUGIN_LANGUAGESERVER_FEATURE_H
