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

#include "../feature.h"
#include "debugging/debugger.h"
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
class dap_feature : public feature, public hlasm_plugin::parser_library::debugging::debug_event_consumer_s
{
public:
    void initialize_feature(const json& client_capabilities) override;

    dap_feature(parser_library::workspace_manager& ws_mngr,
        response_provider& response_provider,
        dap_disconnect_listener* disconnect_listener);

    void on_initialize(const json& requested_seq, const json& args);
    void on_disconnect(const json& request_seq, const json& args);
    void on_launch(const json& request_seq, const json& args);
    void on_set_breakpoints(const json& request_seq, const json& args);
    void on_set_exception_breakpoints(const json& request_seq, const json& args);
    void on_configuration_done(const json& request_seq, const json& args);
    void on_threads(const json& request_seq, const json& args);
    void on_stack_trace(const json& request_seq, const json& args);
    void on_scopes(const json& request_seq, const json& args);
    void on_next(const json& request_seq, const json& args);
    void on_step_in(const json& request_seq, const json& args);
    void on_variables(const json& request_seq, const json& args);
    void on_continue(const json& request_seq, const json& args);

private:
    // Inherited via feature
    void register_methods(std::map<std::string, method>& methods) override;
    json register_capabilities() override;

    // Inherited via debug_event_consumer_s
    void stopped(const std::string& reason, const std::string& addtl_info) override;
    void exited(int exit_code) override;

    hlasm_plugin::parser_library::debugging::debug_config debug_cfg;
    std::unique_ptr<hlasm_plugin::parser_library::debugging::debug_lib_provider> debug_lib_provider;
    std::unique_ptr<hlasm_plugin::parser_library::debugging::debugger> debugger;

    int column_1_based_ = 0;
    int line_1_based_ = 0;
    path_format path_format_ = path_format::PATH;

    dap_disconnect_listener* disconnect_listener_;
};

} // namespace hlasm_plugin::language_server::dap

#endif // HLASMPLUGIN_LANGUAGESERVER_DAP_DAP_FEATURE_H
