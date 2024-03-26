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

#include "parser_library_export.h"
#include "protocol.h"
#include "range.h"
#include "sequence.h"
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
    virtual void stopped(sequence<char> reason, sequence<char> addtl_info) = 0;
    virtual void exited(int exit_code) = 0;
    virtual void mnote(unsigned char level, sequence<char> text) = 0;
    virtual void punch(sequence<char> text) = 0;

    void stopped(std::string_view reason, std::string_view addtl_info)
    {
        stopped(sequence(reason), sequence(addtl_info));
    }
};

class breakpoints_t
{
    class impl;
    impl* pimpl;
    friend class debugger;

public:
    breakpoints_t();
    breakpoints_t(const breakpoints_t&) = delete;
    breakpoints_t(breakpoints_t&&) noexcept;
    breakpoints_t& operator=(const breakpoints_t&) = delete;
    breakpoints_t& operator=(breakpoints_t&&) & noexcept;
    ~breakpoints_t();

    [[nodiscard]] const breakpoint* begin() const;
    [[nodiscard]] const breakpoint* end() const;
    [[nodiscard]] std::size_t size() const;
};

class evaluated_expression_t
{
    class impl;
    impl* pimpl;
    friend class debugger;

public:
    evaluated_expression_t();
    evaluated_expression_t(evaluated_expression_t&&) noexcept;
    evaluated_expression_t& operator=(evaluated_expression_t&&) & noexcept;
    ~evaluated_expression_t();

    [[nodiscard]] sequence<char> result() const noexcept;
    [[nodiscard]] bool is_error() const noexcept;
    [[nodiscard]] std::size_t var_ref() const noexcept;
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

    void launch(sequence<char> source,
        debugger_configuration_provider& dc_provider,
        bool stop_on_entry,
        workspace_manager_response<bool> resp);
    void launch(std::string_view source,
        debugger_configuration_provider& dc_provider,
        bool stop_on_entry,
        workspace_manager_response<bool> resp)
    {
        launch(sequence(source), dc_provider, stop_on_entry, std::move(resp));
    }

    void set_event_consumer(debug_event_consumer* event);

    // User controls for debugging.
    void next();
    void step_in();
    void step_out();
    void disconnect();
    void continue_debug();
    void pause();

    void breakpoints(sequence<char> source, sequence<breakpoint> bps);
    void breakpoints(std::string_view source, sequence<breakpoint> bps) { breakpoints(sequence(source), bps); }
    [[nodiscard]] breakpoints_t breakpoints(sequence<char> source) const;
    [[nodiscard]] breakpoints_t breakpoints(std::string_view source) const { return breakpoints(sequence(source)); }

    // Retrieval of current context.
    stack_frames_t stack_frames() const;
    scopes_t scopes(frame_id_t frame_id) const;
    variables_t variables(var_reference_t var_ref) const;
    evaluated_expression_t evaluate(sequence<char> expr, frame_id_t id = -1);

    void analysis_step(const std::atomic<unsigned char>* yield_indicator);
};

} // namespace hlasm_plugin::parser_library::debugging

#endif // HLASMPLUGIN_PARSERLIBRARY_DEBUGGER_H
