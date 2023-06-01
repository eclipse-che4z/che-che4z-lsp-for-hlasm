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

#ifndef HLASMPLUGIN_LANGUAGESERVER_DAP_DAP_FEATURE_H
#define HLASMPLUGIN_LANGUAGESERVER_DAP_DAP_FEATURE_H

#include <optional>

#include "../feature.h"
#include "debugger.h"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server::dap {

enum class path_format
{
    PATH,
    URI
};

class dap_disconnect_listener
{
protected:
    ~dap_disconnect_listener() = default;

public:
    virtual void disconnected() = 0;
};

// Implements DAP-specific capabilities that are needed for all features:
// path format (path vs URI) and zero-based vs 1-based line and column numbers
class dap_feature : public feature, public hlasm_plugin::parser_library::debugging::debug_event_consumer
{
public:
    void initialize_feature(const nlohmann::json& client_capabilities) override;

    dap_feature(parser_library::debugger_configuration_provider& dc_provider,
        response_provider& response_provider,
        dap_disconnect_listener* disconnect_listener);

    void on_initialize(const request_id& request_seq, const nlohmann::json& args);
    void on_disconnect(const request_id& request_seq, const nlohmann::json& args);
    void on_launch(const request_id& request_seq, const nlohmann::json& args);
    void on_set_breakpoints(const request_id& request_seq, const nlohmann::json& args);
    void on_set_exception_breakpoints(const request_id& request_seq, const nlohmann::json& args);
    void on_configuration_done(const request_id& request_seq, const nlohmann::json& args);
    void on_threads(const request_id& request_seq, const nlohmann::json& args);
    void on_stack_trace(const request_id& request_seq, const nlohmann::json& args);
    void on_scopes(const request_id& request_seq, const nlohmann::json& args);
    void on_next(const request_id& request_seq, const nlohmann::json& args);
    void on_step_in(const request_id& request_seq, const nlohmann::json& args);
    void on_step_out(const request_id& request_seq, const nlohmann::json& args);
    void on_variables(const request_id& request_seq, const nlohmann::json& args);
    void on_continue(const request_id& request_seq, const nlohmann::json& args);
    void on_pause(const request_id& request_seq, const nlohmann::json& args);

    void idle_handler(const std::atomic<unsigned char>* yield_indicator);

private:
    // Inherited via feature
    void register_methods(std::map<std::string, method>& methods) override;
    nlohmann::json register_capabilities() override;

    // Inherited via debug_event_consumer
    void stopped(hlasm_plugin::parser_library::sequence<char> reason,
        hlasm_plugin::parser_library::sequence<char> addtl_info) override;
    void exited(int exit_code) override;

    parser_library::debugger_configuration_provider& dc_provider;
    std::optional<hlasm_plugin::parser_library::debugging::debugger> debugger;

    int column_1_based_ = 0;
    int line_1_based_ = 0;
    path_format client_path_format_ = path_format::PATH;

    dap_disconnect_listener* disconnect_listener_;
};

} // namespace hlasm_plugin::language_server::dap

#endif // HLASMPLUGIN_LANGUAGESERVER_DAP_DAP_FEATURE_H
