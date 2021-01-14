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

#include <optional>
#include <unordered_map>
#include <utility>
#include <variant>

#include "gtest/gtest.h"

#include "debug_event_consumer_s_mock.h"
#include "debugging/debugger.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::debugging;
using namespace hlasm_plugin::parser_library::workspaces;


TEST(debugger, stopped_on_entry)
{
    file_manager_impl file_manager;
    lib_config config;
    workspace ws("test_workspace", file_manager, config);

    debug_event_consumer_s_mock m;
    debug_config cfg;
    debugger d(m, cfg);
    std::string file_name = "test_workspace\\test";
    file_manager.did_open_file(file_name, 0, "   LR 1,2");
    d.launch(file_manager.find_processor_file(file_name), ws, true);
    m.wait_for_stopped();

    auto frames = d.stack_frames();
    ASSERT_EQ(frames.size(), 1U);
    const auto& f = frames.at(0);
    EXPECT_EQ(f.frame_source.path, file_name);
    EXPECT_EQ(f.begin_line, 0U);
    EXPECT_EQ(f.end_line, 0U);
    EXPECT_EQ(f.name, "OPENCODE");
    auto& sc = d.scopes(f.id);
    ASSERT_EQ(sc.size(), 3U);
    EXPECT_EQ(sc.at(0).name, "Globals");
    EXPECT_EQ(sc.at(1).name, "Locals");
    EXPECT_EQ(sc.at(2).name, "Ordinary symbols");
    auto& globs = d.variables(sc.at(0).var_reference);
    EXPECT_EQ(globs.size(), 5U);
    auto& locs = d.variables(sc.at(1).var_reference);
    EXPECT_EQ(locs.size(), 0U);

    d.next();
    m.wait_for_exited();
}

TEST(debugger, disconnect)
{
    file_manager_impl file_manager;
    lib_config config;
    workspace ws("test_workspace", file_manager, config);

    debug_event_consumer_s_mock m;
    debug_config cfg;
    debugger d(m, cfg);
    std::string file_name = "test_workspace\\test";
    file_manager.did_open_file(file_name, 0, "   LR 1,2");
    d.launch(file_manager.find_processor_file(file_name), ws, true);
    m.wait_for_stopped();

    d.disconnect();
}

using namespace context;
class test_var_value
{
public:
    test_var_value(std::unordered_map<std::string, std::shared_ptr<test_var_value>> vec)
        : children_(vec)
        , ignore_(false)
    {}
    test_var_value(A_t str)
        : data_(std::to_string(str))
        , ignore_(false)
    {}
    test_var_value(B_t str)
        : data_(str ? "TRUE" : "FALSE")
        , ignore_(false)
    {}
    test_var_value(std::string str)
        : data_(str)
        , ignore_(false)
    {}
    test_var_value(std::string str, std::unordered_map<std::string, std::shared_ptr<test_var_value>> vec)
        : children_(vec)
        , data_(str)
        , ignore_(false)
    {}
    test_var_value(const char* cstr)
        : data_(std::string(cstr))
        , ignore_(false)
    {}
    test_var_value(const char* cstr, std::unordered_map<std::string, std::shared_ptr<test_var_value>> vec)
        : children_(vec)
        , data_(std::string(cstr))
        , ignore_(false)
    {}
    test_var_value()
        : ignore_(true)
    {}

    test_var_value(std::string str, set_type)
        : data_(str)
    {}

    bool check(debugger& d, const debugging::variable& var) const
    {
        if (ignore_)
            return true;

        if (!children_.empty())
        {
            if (var.is_scalar())
                return false;
            if (var.size() != children_.size())
                return false;
            auto& actual_children = d.variables(var.var_reference);
            if (actual_children.size() != children_.size())
                return false;
            for (auto& actual_ch : actual_children)
            {
                auto found = children_.find(actual_ch->get_name());
                if (found == children_.end())
                    return false;
                if (found->first != actual_ch->get_name())
                    return false;
                if (!found->second->check(d, *actual_ch))
                    return false;
            }
        }

        if (data_)
            return *data_ == var.get_value();
        else
            return var.get_value() == "";
    }

private:
    std::unordered_map<std::string, std::shared_ptr<test_var_value>> children_;
    std::optional<std::string> data_;
    bool ignore_;
};

struct frame_vars
{
    frame_vars(std::unordered_map<std::string, test_var_value> globals,
        std::unordered_map<std::string, test_var_value> locals,
        std::unordered_map<std::string, test_var_value> ords)
        : globals(std::move(globals))
        , locals(std::move(locals))
        , ord_syms(std::move(ords))
    {
        this->globals["&SYSDATE"];
        this->globals["&SYSDATC"];
        this->globals["&SYSTIME"];
        this->globals["&SYSPARM"];
        this->globals["&SYSOPT_RENT"];
    }
    std::unordered_map<std::string, test_var_value> globals;
    std::unordered_map<std::string, test_var_value> locals;
    std::unordered_map<std::string, test_var_value> ord_syms;
};

bool check_vars(debugger& d,
    const std::vector<debugging::variable_ptr>& vars,
    const std::unordered_map<std::string, test_var_value>& exp_vars)
{
    if (vars.size() != exp_vars.size())
        return false;
    for (size_t i = 0; i < vars.size(); ++i)
    {
        auto it = exp_vars.find(vars[i]->get_name());
        if (it == exp_vars.end())
            return false;
        if (!it->second.check(d, *vars[i]))
            return false;
    }
    return true;
}

bool check_step(
    debugger& d, const std::vector<debugging::stack_frame>& exp_frames, const std::vector<frame_vars>& exp_frame_vars)
{
    auto frames = d.stack_frames();
    if (frames != exp_frames)
        return false;
    if (frames.size() != exp_frame_vars.size())
        return false;
    for (size_t i = 0; i < frames.size(); ++i)
    {
        auto& sc = d.scopes(frames.at(i).id);
        if (sc.size() != 3U)
            return false;
        auto& globs = d.variables(sc.at(0).var_reference);
        if (!check_vars(d, globs, exp_frame_vars[i].globals))
            return false;
        auto& locs = d.variables(sc.at(1).var_reference);
        if (!check_vars(d, locs, exp_frame_vars[i].locals))
            return false;
        auto& ords = d.variables(sc.at(2).var_reference);
        if (!check_vars(d, ords, exp_frame_vars[i].ord_syms))
            return false;
    }
    return true;
}

class workspace_mock : public workspace
{
    lib_config config;

public:
    workspace_mock(file_manager& file_mngr)
        : workspace(file_mngr, config)
    {}

    virtual parse_result parse_library(
        const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data) override
    {
        std::shared_ptr<processor> found = get_file_manager().add_processor_file(library);
        if (found)
            return found->parse_no_lsp_update(*this, hlasm_ctx, data);

        return false;
    }
};

TEST(debugger, test)
{
    using list = std::unordered_map<std::string, std::shared_ptr<test_var_value>>;

    std::string open_code = R"(
&VAR    SETA 2
&BOOL   SETB (&VAR EQ 2)
&STR    SETC 'SOMETHING'

        MACRO
        MAC &VAR
        LR 1,1
        COPY COPY1

        MEND

10      MAC 13
        LR 1,1
)";
    std::string copy1_filename = "COPY1";
    std::string copy1_source = R"(
        COPY COPY2
)";

    std::string copy2_filename = "COPY2";
    std::string copy2_source = R"(

        ANOP
)";


    file_manager_impl file_manager;
    file_manager.did_open_file(copy1_filename, 0, copy1_source);
    file_manager.did_open_file(copy2_filename, 0, copy2_source);
    workspace_mock lib_provider(file_manager);

    debug_event_consumer_s_mock m;
    debug_config cfg;
    debugger d(m, cfg);
    std::string filename = "ws\\test";
    file_manager.did_open_file(filename, 0, open_code);
    d.launch(file_manager.find_processor_file(filename), lib_provider, true);
    m.wait_for_stopped();
    std::vector<debugging::stack_frame> exp_frames { { 1, 1, 0, "OPENCODE", filename } };
    std::vector<frame_vars> exp_frame_vars { { {}, {}, {} } };
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.next();
    m.wait_for_stopped();
    exp_frame_vars[0].locals.emplace("&VAR", 2);
    exp_frames[0].begin_line = exp_frames[0].end_line = 2;
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.next();
    m.wait_for_stopped();
    exp_frames[0].begin_line = exp_frames[0].end_line = 3;
    exp_frame_vars[0].locals.emplace("&BOOL", true);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.next();
    m.wait_for_stopped();
    exp_frames[0].begin_line = exp_frames[0].end_line = 5;
    exp_frame_vars[0].locals.emplace("&STR", "SOMETHING");
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.next();
    m.wait_for_stopped();
    exp_frames[0].begin_line = exp_frames[0].end_line = 12;
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.step_in();
    m.wait_for_stopped();
    exp_frames.insert(exp_frames.begin(), debugging::stack_frame(7, 7, 0, "MACRO", filename));
    exp_frame_vars.insert(exp_frame_vars.begin(),
        frame_vars(std::unordered_map<std::string, test_var_value> {}, // empty globals
            std::unordered_map<std::string, test_var_value> {
                // macro locals
                { "&SYSLIST",
                    test_var_value("(10,13)",
                        list { { "0", std::make_shared<test_var_value>("10") },
                            { "1", std::make_shared<test_var_value>("13") } }) },
                { "&SYSECT", "" },
                { "&SYSNDX", "0000" },
                { "&SYSSTYP", "" },
                { "&SYSLOC", "" },
                { "&SYSNEST", 1 },
                { "&SYSMAC", test_var_value() },
                { "&VAR", "13" },
            },
            {} // empty ord symbols
            ));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.step_in();
    m.wait_for_stopped();
    exp_frames[0].begin_line = exp_frames[0].end_line = 8;
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.step_in();
    m.wait_for_stopped();
    exp_frames.insert(exp_frames.begin(), debugging::stack_frame(1, 1, 0, "COPY", copy1_filename));
    exp_frame_vars.insert(exp_frame_vars.begin(), exp_frame_vars[0]);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.step_in();
    m.wait_for_stopped();
    exp_frames.insert(exp_frames.begin(), debugging::stack_frame(2, 2, 0, "COPY", copy2_filename));
    exp_frame_vars.insert(exp_frame_vars.begin(), exp_frame_vars[0]);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.next();
    m.wait_for_stopped();
    exp_frames.erase(exp_frames.begin(), exp_frames.begin() + 2);
    exp_frame_vars.erase(exp_frame_vars.begin(), exp_frame_vars.begin() + 2);
    exp_frames[0].begin_line = exp_frames[0].end_line = 10;
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.next();
    m.wait_for_stopped();
    exp_frames.erase(exp_frames.begin());
    exp_frame_vars.erase(exp_frame_vars.begin());
    exp_frames[0].begin_line = exp_frames[0].end_line = 13;
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.disconnect();
}

TEST(debugger, var_symbol_array)
{
    using list = std::unordered_map<std::string, std::shared_ptr<test_var_value>>;

    std::string open_code = R"(
&VARP(30) SETA 1,456,48,7
 LR 1,1
)";


    file_manager_impl file_manager;
    workspace_mock lib_provider(file_manager);
    debug_event_consumer_s_mock m;
    debug_config cfg;
    debugger d(m, cfg);
    std::string filename = "ws\\test";
    file_manager.did_open_file(filename, 0, open_code);

    d.launch(file_manager.find_processor_file(filename), lib_provider, true);
    m.wait_for_stopped();
    std::vector<debugging::stack_frame> exp_frames { { 1, 1, 0, "OPENCODE", filename } };
    std::vector<frame_vars> exp_frame_vars { { {}, {}, {} } };
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.next();
    m.wait_for_stopped();
    exp_frame_vars[0].locals.emplace("&VARP",
        list { { "30", std::make_shared<test_var_value>(1) },
            { "31", std::make_shared<test_var_value>(456) },
            { "32", std::make_shared<test_var_value>(48) },
            { "33", std::make_shared<test_var_value>(7) } });
    exp_frames[0].begin_line = exp_frames[0].end_line = 2;
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.next();
    m.wait_for_exited();
}

TEST(debugger, ordinary)
{
    using list = std::unordered_map<std::string, std::shared_ptr<test_var_value>>;

    std::string open_code = R"(
A EQU 13
B EQU A
)";


    file_manager_impl file_manager;
    workspace_mock lib_provider(file_manager);
    debug_event_consumer_s_mock m;
    debug_config cfg;
    debugger d(m, cfg);
    std::string filename = "ws\\test";
    file_manager.did_open_file(filename, 0, open_code);

    d.launch(file_manager.find_processor_file(filename), lib_provider, true);
    m.wait_for_stopped();
    std::vector<debugging::stack_frame> exp_frames { { 1, 1, 0, "OPENCODE", filename } };
    std::vector<frame_vars> exp_frame_vars { { {}, {}, {} } };
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.next();
    m.wait_for_stopped();
    exp_frame_vars[0].ord_syms.emplace("A",
        test_var_value("13",
            list { { "L", std::make_shared<test_var_value>("1") }, { "T", std::make_shared<test_var_value>("U") } }));
    exp_frames[0].begin_line = exp_frames[0].end_line = 2;
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.disconnect();
}

TEST(debugger, concurrent_next_and_file_change)
{
    std::string open_code = R"(
        LR 1,1
    COPY COPY1
)";
    std::string copy1_filename = "COPY1";
    std::string copy1_source = R"(
        LR 1,1
        LR 1,1
        LR 1,1
        LR 1,1
        LR 1,1
        LR 1,1
)";


    file_manager_impl file_manager;
    file_manager.did_open_file(copy1_filename, 0, copy1_source);
    workspace_mock lib_provider(file_manager);

    debug_event_consumer_s_mock m;
    debug_config cfg;
    debugger d(m, cfg);
    std::string filename = "ws\\test";

    file_manager.did_open_file(filename, 0, open_code);
    d.launch(file_manager.find_processor_file(filename), lib_provider, true);
    m.wait_for_stopped();
    std::string new_string = "SOME NEW FILE DOES NOT MATTER";
    std::vector<document_change> chs;
    chs.emplace_back(new_string.c_str(), new_string.size());
    d.next();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::thread t([&file_manager, &copy1_filename, &chs]() {
        file_manager.did_change_file(copy1_filename, 0, chs.data(), chs.size());
    });
    m.wait_for_stopped();


    d.disconnect();

    t.join();
}
