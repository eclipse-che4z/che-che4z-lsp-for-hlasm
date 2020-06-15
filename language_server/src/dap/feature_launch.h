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

#ifndef HLASMPLUGIN_LANGUAGESERVER_DAP_FEATURE_LAUNCH_H
#define HLASMPLUGIN_LANGUAGESERVER_DAP_FEATURE_LAUNCH_H

#include "dap_feature.h"

namespace hlasm_plugin::language_server::dap {
// Implements all the events and requests from DAP protocol.
class feature_launch : public dap_feature, public parser_library::debug_event_consumer
{
public:
    feature_launch(parser_library::workspace_manager& ws_mngr, response_provider& response_provider);

    // Inherited via dap_feature
    virtual void register_methods(std::map<std::string, method>& methods) override;
    virtual json register_capabilities() override;

    virtual ~feature_launch();

private:
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

    // Inherited via debug_event_consumer
    virtual void stopped(const char* reason, const char* addtl_info) override;
    virtual void exited(int exit_code) override;
    virtual void terminated() override;
};


} // namespace hlasm_plugin::language_server::dap
#endif
