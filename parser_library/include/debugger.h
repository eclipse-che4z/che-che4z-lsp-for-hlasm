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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGER_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGER_H

#include <span>

#include "debug_types.h"
#include "workspace_manager.h"
#include "workspace_manager_response.h"

namespace hlasm_plugin::parser_library::debugging {
struct debugger_configuration;

// Interface that a listener can implement to be notified about debugging events.
class debug_event_consumer
{
protected:
    ~debug_event_consumer() = default;

public:
    virtual void stopped(std::string_view reason, std::string_view addtl_info) = 0;
    virtual void exited(int exit_code) = 0;
    virtual void mnote(unsigned char level, std::string_view text) = 0;
    virtual void punch(std::string_view text) = 0;
};

// Implements DAP for macro tracing. Starts analyzer in a separate thread
// then controls the flow of analyzer by implementing processing_tracer
// interface.
class debugger
{
    class impl;
    impl* pimpl;

public:
    debugger();
    debugger(debugger&&) noexcept;
    debugger& operator=(debugger&&) & noexcept;
    ~debugger();

    void launch(std::string_view source,
        debugger_configuration_provider& dc_provider,
        bool stop_on_entry,
        workspace_manager_response<bool> resp);

    void set_event_consumer(debug_event_consumer* event);

    // User controls for debugging.
    void next();
    void step_in();
    void step_out();
    void disconnect();
    void continue_debug();
    void pause();

    void breakpoints(std::string_view source, std::span<const breakpoint> bps);
    [[nodiscard]] std::span<const breakpoint> breakpoints(std::string_view source) const;

    void function_breakpoints(std::span<const function_breakpoint> bps);

    // Retrieval of current context.
    std::span<const stack_frame> stack_frames() const;
    std::span<const scope> scopes(frame_id_t frame_id) const;
    std::span<const variable> variables(var_reference_t var_ref) const;
    evaluated_expression evaluate(std::string_view expr, frame_id_t id = -1);

    void analysis_step(const std::atomic<unsigned char>* yield_indicator);
};

} // namespace hlasm_plugin::parser_library::debugging

#endif // HLASMPLUGIN_PARSERLIBRARY_DEBUGGER_H
