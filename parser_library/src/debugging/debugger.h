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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGER_DEBUGGER_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGER_DEBUGGER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "context/hlasm_context.h"
#include "debug_lib_provider.h"
#include "debug_types.h"
#include "processing/statement_analyzers/statement_analyzer.h"
#include "workspaces/file_manager.h"
#include "workspaces/processor.h"

namespace hlasm_plugin::parser_library::debugging {

// Interface that a listener can implement to be notified about debugging events.
class debug_event_consumer_s
{
public:
    virtual void stopped(const std::string& reason, const std::string& addtl_info) = 0;
    virtual void exited(int exit_code) = 0;
};

// Represents configuration of breakpoints. Must be separated, because
// breakpoints can be set even before debugger starts. Provides thread
// safe access.
class debug_config
{
public:
    void set_breakpoints(breakpoints breakpoints);
    breakpoints get_breakpoints(const std::string& source);

private:
    std::unordered_map<std::string, breakpoints> bpoints_;
    std::mutex bpoints_mutex_;
};

// Implements DAP for macro tracing. Starts analyzer in a separate thread
// then controls the flow of analyzer by implementing processing_tracer
// interface.
class debugger : public processing::statement_analyzer
{
public:
    debugger(debug_event_consumer_s& event_consumer, debug_config& debug_cfg);

    void launch(workspaces::processor_file_ptr open_code, workspaces::parse_lib_provider& provider, bool stop_on_entry);

    void analyze(const context::hlasm_statement& statement,
        processing::statement_provider_kind prov_kind,
        processing::processing_kind proc_kind) override;

    // User controls of debugging.
    void next();
    void step_in();
    void disconnect();
    void continue_debug();

    // Retrieval of current context.
    const std::vector<stack_frame>& stack_frames();
    const std::vector<scope>& scopes(frame_id_t frame_id);
    const std::vector<variable_ptr>& variables(var_reference_t var_ref);

    ~debugger();

private:
    // Creates analyzer and starts parsing
    void debug_start(workspaces::processor_file_ptr open_code, workspaces::parse_lib_provider* provider);

    std::unique_ptr<std::thread> thread_;

    // these are used in conditional variable to stop execution
    // of analyzer and wait for user input
    std::mutex control_mtx;
    std::condition_variable con_var;
    std::atomic<bool> continue_ = false;

    std::mutex variable_mtx_;
    bool debug_ended_ = false;

    // Specifies whether the debugger stops on the next statement call.
    std::atomic<bool> stop_on_next_stmt_ = false;
    std::atomic<bool> step_over_ = false;
    size_t step_over_depth_;
    // Range of statement that is about to be processed by analyzer.
    range next_stmt_range_;

    // True, if disconnect request was received
    bool disconnected_ = false;

    // Cancellation token for parsing
    std::atomic<bool> cancel_ = false;

    // Provides a way to inform outer world about debugger events
    debug_event_consumer_s& event_;

    // Debugging information retrieval
    context::hlasm_context* ctx_;
    std::string opencode_source_path_;
    std::vector<stack_frame> stack_frames_;
    std::vector<scope> scopes_;

    std::unordered_map<size_t, std::vector<variable_ptr>> variables_;
    size_t next_var_ref_ = 1;
    context::processing_stack_t proc_stack_;

    debug_config& cfg_;
};

} // namespace hlasm_plugin::parser_library::debugging

#endif // !HLASMPLUGIN_PARSERLIBRARY_DEBUGGER_DEBUGGER_H
