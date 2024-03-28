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

#include <functional>
#include <optional>
#include <regex>
#include <string_view>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../workspace/empty_configs.h"
#include "../workspace_manager_response_mock.h"
#include "compiler_options.h"
#include "debug_event_consumer_s_mock.h"
#include "debugger.h"
#include "debugging/debug_types.h"
#include "debugging/debugger_configuration.h"
#include "protocol.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

using namespace ::testing;
using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::debugging;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

namespace {

struct expected_stack_frame
{
    expected_stack_frame(size_t begin_line, size_t end_line, uint32_t id, std::string name, std::string source)
        : begin_line(begin_line)
        , end_line(end_line)
        , id(id)
        , name(std::move(name))
        , frame_source(std::move(source))
    {}
    expected_stack_frame(size_t begin_line,
        size_t end_line,
        uint32_t id,
        std::string name,
        std::function<bool(std::string_view)> source_matcher)
        : begin_line(begin_line)
        , end_line(end_line)
        , id(id)
        , name(std::move(name))
        , frame_source(std::move(source_matcher))
    {}
    size_t begin_line;
    size_t end_line;
    uint32_t id;
    std::string name;
    std::variant<std::string, std::function<bool(std::string_view)>> frame_source;

    static bool check_frame_source(const std::string& exp, std::string_view uri) { return exp == uri; }
    static bool check_frame_source(const std::function<bool(std::string_view)>& exp, std::string_view uri)
    {
        return exp(uri);
    }
    bool check_frame_source(std::string_view uri) const
    {
        return std::visit([uri](const auto& v) { return check_frame_source(v, uri); }, frame_source);
    }
};

class debugger_configuration_provider_mock : public debugger_configuration_provider
{
public:
    MOCK_METHOD(void,
        provide_debugger_configuration,
        (sequence<char> document_uri, workspace_manager_response<debugger_configuration> conf),
        (override));
};
} // namespace

TEST(debugger, stopped_on_entry)
{
    file_manager_impl file_manager;
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration).WillRepeatedly(Invoke([&file_manager](auto, auto r) {
        r.provide({ .fm = &file_manager });
    }));

    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string file_name = "test_workspace\\test";
    resource_location file_loc(file_name);

    file_manager.did_open_file(file_loc, 0, "   LR 1,2");

    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(file_name.c_str(), dc_provider, true, resp);

    m.wait_for_stopped();

    auto frames = d.stack_frames();
    ASSERT_EQ(frames.size(), 1U);
    const auto f = frames.item(0);
    EXPECT_EQ(std::string_view(f.source_file.uri), file_loc.get_uri());
    EXPECT_EQ(f.source_range.start.line, 0U);
    EXPECT_EQ(f.source_range.end.line, 0U);
    EXPECT_EQ(std::string_view(f.name), "OPENCODE");
    auto sc = d.scopes(f.id);
    ASSERT_EQ(sc.size(), 3U);
    EXPECT_EQ(std::string_view(sc.item(0).name), "Globals");
    EXPECT_EQ(std::string_view(sc.item(1).name), "Locals");
    EXPECT_EQ(std::string_view(sc.item(2).name), "Ordinary symbols");
    auto globs = d.variables(sc.item(0).variable_reference);
    EXPECT_EQ(globs.size(), 12U);
    auto locs = d.variables(sc.item(1).variable_reference);
    EXPECT_EQ(locs.size(), 0U);

    d.next();
    m.wait_for_exited();
}

TEST(debugger, disconnect)
{
    file_manager_impl file_manager;
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration).WillRepeatedly(Invoke([&file_manager](auto, auto r) {
        r.provide({ .fm = &file_manager });
    }));

    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string file_name = "test_workspace\\test";
    resource_location file_loc(file_name);

    file_manager.did_open_file(file_loc, 0, "   LR 1,2");
    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(file_name.c_str(), dc_provider, true, resp);
    m.wait_for_stopped();

    d.disconnect();
}

using namespace context;
class test_var_value
{
public:
    test_var_value(std::unordered_map<std::string, std::shared_ptr<test_var_value>> vec)
        : children_(vec)
    {}
    test_var_value(A_t str)
        : data_(std::to_string(str))
    {}
    test_var_value(B_t str)
        : data_(str ? "TRUE" : "FALSE")
    {}
    test_var_value(std::string str)
        : data_(str)
    {}
    test_var_value(std::string str, std::unordered_map<std::string, std::shared_ptr<test_var_value>> vec)
        : children_(vec)
        , data_(str)
    {}
    test_var_value(const char* cstr)
        : data_(std::string(cstr))
    {}
    test_var_value(const char* cstr, std::unordered_map<std::string, std::shared_ptr<test_var_value>> vec)
        : children_(vec)
        , data_(std::string(cstr))
    {}
    test_var_value()
        : ignore_(true)
    {}
    test_var_value(std::string str, set_type)
        : data_(str)
    {}

    bool check(debugger& d, const hlasm_plugin::parser_library::variable& var) const
    {
        if (ignore_)
            return true;

        if (has_children(var) && !check_children(d, var))
        {
            return false;
        }

        if (data_)
            return *data_ == std::string(var.value);
        else
            return var.value.size() == 0;
    }

    bool has_children(const hlasm_plugin::parser_library::variable& var) const { return var.variable_reference; }

    bool check_children(debugger& d, const hlasm_plugin::parser_library::variable& var) const
    {
        auto child_vars = var.variable_reference;
        if (!child_vars)
            return false;
        auto actual_children = d.variables(child_vars);
        if (actual_children.size() != children_.size())
            return false;
        for (const auto& actual_ch : actual_children)
        {
            std::string actual_ch_name(actual_ch.name);
            auto found = children_.find(actual_ch_name);
            if (found == children_.end())
                return false;
            if (found->first != actual_ch_name)
                return false;
            if (!found->second->check(d, actual_ch))
                return false;
        }

        return true;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<test_var_value>> children_;
    std::optional<std::string> data_;
    bool ignore_ = false;
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
        this->globals["&SYSOPT_RENT"];
        this->globals["&SYSOPT_XOBJECT"];
        this->globals["&SYSPARM"];
        this->globals["&SYSSTMT"];
        this->globals["&SYSTEM_ID"];
        this->globals["&SYSVER"];
        this->globals["&SYSASM"];
        this->globals["&SYSM_SEV"];
        this->globals["&SYSM_HSEV"];
    }
    std::unordered_map<std::string, test_var_value> globals;
    std::unordered_map<std::string, test_var_value> locals;
    std::unordered_map<std::string, test_var_value> ord_syms;
};

struct frame_vars_ignore_sys_vars : public frame_vars
{
    frame_vars_ignore_sys_vars(std::unordered_map<std::string, test_var_value> globals,
        std::unordered_map<std::string, test_var_value> locals,
        std::unordered_map<std::string, test_var_value> ords)
        : frame_vars(std::move(globals), std::move(locals), std::move(ords))
    {
        this->locals["&SYSLIST"];
        this->locals["&SYSECT"];
        this->locals["&SYSLOC"];
        this->locals["&SYSNDX"];
        this->locals["&SYSSTYP"];
        this->locals["&SYSNEST"];
        this->locals["&SYSMAC"];
        this->locals["&SYSIN_DSN"];
        this->locals["&SYSIN_MEMBER"];
        this->locals["&SYSCLOCK"];
    }
};

bool check_vars(debugger& d,
    const std::vector<hlasm_plugin::parser_library::variable>& vars,
    const std::unordered_map<std::string, test_var_value>& exp_vars)
{
    if (vars.size() != exp_vars.size())
        return false;
    for (auto& var : vars)
    {
        auto it = exp_vars.find(std::string(var.name));
        if (it == exp_vars.end())
            return false;
        if (!it->second.check(d, var))
            return false;
    }
    return true;
}

bool check_step(
    debugger& d, const std::vector<expected_stack_frame>& exp_frames, const std::vector<frame_vars>& exp_frame_vars)
{
    auto frames = d.stack_frames();
    if (frames.size() != exp_frames.size())
        return false;
    for (size_t i = 0; i != exp_frames.size(); ++i)
    {
        auto f = frames.item(i);
        if (exp_frames[i].begin_line != f.source_range.start.line)
            return false;
        if (exp_frames[i].end_line != f.source_range.end.line)
            return false;
        if (!exp_frames[i].check_frame_source(std::string_view(f.source_file.uri)))
            return false;
        if (exp_frames[i].id != f.id)
            return false;
        if (exp_frames[i].name != std::string_view(f.name))
            return false;
    }
    if (frames.size() != exp_frame_vars.size())
        return false;
    for (size_t i = 0; i < frames.size(); ++i)
    {
        auto sc = d.scopes(frames.item(i).id);
        if (sc.size() != 3U)
            return false;
        using variables = std::vector<hlasm_plugin::parser_library::variable>;
        variables globs(d.variables(sc.item(0).variable_reference));
        if (!check_vars(d, globs, exp_frame_vars[i].globals))
            return false;
        variables locs(d.variables(sc.item(1).variable_reference));
        if (!check_vars(d, locs, exp_frame_vars[i].locals))
            return false;
        variables ords(d.variables(sc.item(2).variable_reference));
        if (!check_vars(d, ords, exp_frame_vars[i].ord_syms))
            return false;
    }
    return true;
}

namespace {
void step_over_by(size_t number_of_steps,
    debugger& d,
    debug_event_consumer_s_mock& m,
    std::vector<expected_stack_frame>& exp_stack_frames,
    size_t line)
{
    exp_stack_frames[0].begin_line = exp_stack_frames[0].end_line = line;

    while (number_of_steps > 0)
    {
        d.next();
        m.wait_for_stopped();
        number_of_steps--;
    }
}

void step_into(debugger& d,
    debug_event_consumer_s_mock& m,
    std::vector<expected_stack_frame>& exp_stack_frames,
    size_t line,
    std::string name,
    std::variant<resource_location, std::function<bool(std::string_view)>> source)
{
    uint32_t next_frame_id = exp_stack_frames.empty() ? 0 : exp_stack_frames[0].id + 1;

    if (std::holds_alternative<resource_location>(source))
        exp_stack_frames.insert(exp_stack_frames.begin(),
            expected_stack_frame(line, line, next_frame_id, name, std::get<resource_location>(source).get_uri()));
    else
        exp_stack_frames.insert(exp_stack_frames.begin(),
            expected_stack_frame(
                line, line, next_frame_id, name, std::get<std::function<bool(std::string_view)>>(source)));

    d.step_in();
    m.wait_for_stopped();
}

void erase_frames_from_top(size_t number_of_frames,
    std::vector<expected_stack_frame>& exp_stack_frames,
    std::vector<frame_vars>& exp_frame_vars)
{
    exp_stack_frames.erase(exp_stack_frames.begin(), exp_stack_frames.begin() + number_of_frames);
    exp_frame_vars.erase(exp_frame_vars.begin(), exp_frame_vars.begin() + number_of_frames);
}

void step_out(
    debugger& d, debug_event_consumer_s_mock& m, std::vector<expected_stack_frame>& exp_stack_frames, size_t line)
{
    d.step_out();
    m.wait_for_stopped();
    exp_stack_frames.erase(exp_stack_frames.begin());

    exp_stack_frames[0].begin_line = exp_stack_frames[0].end_line = line;
}
} // namespace

class workspace_mock : public workspace
{
    lib_config config;
    shared_json global_settings = make_empty_shared_json();

public:
    workspace_mock(file_manager& file_mngr)
        : workspace(file_mngr, config, global_settings)
    {}

    std::vector<std::shared_ptr<library>> get_libraries(const resource_location&) const override
    {
        struct debugger_mock_library : library
        {
            file_manager& fm;
            hlasm_plugin::utils::task refresh() override
            {
                assert(false);
                return {};
            }
            hlasm_plugin::utils::task prefetch() override { return {}; }

            std::vector<std::string> list_files() override
            {
                assert(false);
                return {};
            }

            bool has_file(std::string_view file, resource_location* url)
            {
                bool result = !!fm.find(resource_location(file));
                if (result && url)
                    *url = resource_location(file);
                return result;
            }

            void copy_diagnostics(std::vector<diagnostic_s>&) const override { assert(false); }

            const resource_location& get_location() const
            {
                assert(false);
                static resource_location rl("");
                return rl;
            };

            bool has_cached_content() const override { return false; }

            debugger_mock_library(file_manager& fm)
                : fm(fm)
            {}
        };

        return { std::make_shared<debugger_mock_library>(get_file_manager()) };
    }
    asm_option get_asm_options(const resource_location&) const override { return { "SEVEN", "" }; }
    std::vector<preprocessor_options> get_preprocessor_options(const resource_location&) const override { return {}; }
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
    resource_location copy1_file_loc(copy1_filename);
    std::string copy1_source = R"(
        COPY COPY2
)";

    std::string copy2_filename = "COPY2";
    resource_location copy2_file_loc(copy2_filename);
    std::string copy2_source = R"(

        ANOP
)";


    file_manager_impl file_manager;
    file_manager.did_open_file(copy1_file_loc, 0, copy1_source);
    file_manager.did_open_file(copy2_file_loc, 0, copy2_source);
    workspace_mock lib_provider(file_manager);
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration)
        .WillRepeatedly(Invoke([&file_manager, &lib_provider](auto uri, auto r) {
            resource_location res = resource_location(std::string_view(uri));
            r.provide({
                .fm = &file_manager,
                .libraries = lib_provider.get_libraries(res),
                .workspace_uri = resource_location(lib_provider.uri()),
                .opts = lib_provider.get_asm_options(res),
                .pp_opts = lib_provider.get_preprocessor_options(res),
            });
        }));

    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string filename = "ws\\test";
    resource_location file_loc(filename);

    file_manager.did_open_file(file_loc, 0, open_code);
    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(filename, dc_provider, true, resp);
    m.wait_for_stopped();
    std::vector<expected_stack_frame> exp_frames { { 1, 1, 0, "OPENCODE", file_loc.get_uri() } };
    std::vector<frame_vars> exp_frame_vars { { {}, {}, {} } };
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 2);
    exp_frame_vars[0].locals.emplace("&VAR", 2);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 3);
    exp_frame_vars[0].locals.emplace("&BOOL", true);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 5);
    exp_frame_vars[0].locals.emplace("&STR", "SOMETHING");
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 12);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_into(d, m, exp_frames, 7, "MACRO", file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(),
        frame_vars(std::unordered_map<std::string, test_var_value> { {
                       "&SYSPARM",
                       test_var_value("SEVEN"),
                   } },
            std::unordered_map<std::string, test_var_value> {
                // macro locals
                {
                    "&SYSLIST",
                    test_var_value("(10,13)",
                        list {
                            { "0", std::make_shared<test_var_value>("10") },
                            { "1", std::make_shared<test_var_value>("13") },
                        }),
                },
                { "&SYSECT", "" },
                { "&SYSNDX", "0001" },
                { "&SYSSTYP", "" },
                { "&SYSLOC", "" },
                { "&SYSNEST", 1 },
                { "&SYSMAC", test_var_value() },
                { "&SYSIN_DSN", "" },
                { "&SYSIN_MEMBER", "" },
                { "&VAR", "13" },
                { "&SYSCLOCK", test_var_value() },
            },
            {} // empty ord symbols
            ));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 8);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_into(d, m, exp_frames, 1, "COPY", copy1_file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(), exp_frame_vars[0]);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_into(d, m, exp_frames, 2, "COPY", copy2_file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(), exp_frame_vars[0]);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    erase_frames_from_top(2, exp_frames, exp_frame_vars);
    step_over_by(1, d, m, exp_frames, 10);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    erase_frames_from_top(1, exp_frames, exp_frame_vars);
    step_over_by(1, d, m, exp_frames, 13);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.disconnect();
}

TEST(debugger, sysstmt)
{
    std::string open_code = R"(
        LR 1,1
        LR 1,1
        LR 1,1
)";

    file_manager_impl file_manager;
    workspace_mock lib_provider(file_manager);
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration)
        .WillRepeatedly(Invoke([&file_manager, &lib_provider](auto uri, auto r) {
            resource_location res = resource_location(std::string_view(uri));
            r.provide({
                .fm = &file_manager,
                .libraries = lib_provider.get_libraries(res),
                .workspace_uri = resource_location(lib_provider.uri()),
                .opts = lib_provider.get_asm_options(res),
                .pp_opts = lib_provider.get_preprocessor_options(res),
            });
        }));

    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string filename = "ws\\test";
    resource_location file_loc(filename);

    file_manager.did_open_file(file_loc, 0, open_code);
    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(filename, dc_provider, true, resp);
    m.wait_for_stopped();
    std::vector<expected_stack_frame> exp_frames { { 1, 1, 0, "OPENCODE", file_loc.get_uri() } };
    std::vector<frame_vars> exp_frame_vars { { { {
                                                   "&SYSSTMT",
                                                   "00000003",
                                               } },
        {},
        {} } };
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 2);
    exp_frame_vars[0].globals["&SYSSTMT"] = "00000004";
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 3);
    exp_frame_vars[0].globals["&SYSSTMT"] = "00000005";
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.disconnect();
}

TEST(debugger, sysmac_syslist)
{
    using list = std::unordered_map<std::string, std::shared_ptr<test_var_value>>;
    std::string open_code = R"(
   MACRO
   MAC_IN
   LR 1,1
   COPY COPY1
   MEND
   
   MACRO
   MAC_1
A  MAC_IN ()
   MEND
   
   MACRO
   MAC_2
   MEND
   
   MAC_1
   MAC_2 3456,(),,((1,('a','b')),2),((()))
)";

    std::string copy1_filename = "COPY1";
    resource_location copy1_file_loc(copy1_filename);
    std::string copy1_source = R"(
           LR 1,1
)";

    file_manager_impl file_manager;
    file_manager.did_open_file(copy1_file_loc, 0, copy1_source);
    workspace_mock lib_provider(file_manager);
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration)
        .WillRepeatedly(Invoke([&file_manager, &lib_provider](auto uri, auto r) {
            resource_location res = resource_location(std::string_view(uri));
            r.provide({
                .fm = &file_manager,
                .libraries = lib_provider.get_libraries(res),
                .workspace_uri = resource_location(lib_provider.uri()),
                .opts = lib_provider.get_asm_options(res),
                .pp_opts = lib_provider.get_preprocessor_options(res),
            });
        }));

    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string filename = "ws\\test";
    resource_location file_loc(filename);

    file_manager.did_open_file(file_loc, 0, open_code);
    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(filename, dc_provider, true, resp);
    m.wait_for_stopped();
    std::vector<expected_stack_frame> exp_frames { { 1, 1, 0, "OPENCODE", file_loc.get_uri() } };
    std::vector<frame_vars> exp_frame_vars { { {}, {}, {} } };

    step_over_by(3, d, m, exp_frames, 16);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_into(d, m, exp_frames, 9, "MACRO", file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(),
        frame_vars_ignore_sys_vars({},
            std::unordered_map<std::string, test_var_value> {
                // macro locals
                {
                    "&SYSLIST",
                    test_var_value("()",
                        list {
                            { "0", std::make_shared<test_var_value>("") },
                        }),
                },
                {

                    "&SYSMAC",
                    test_var_value("(MAC_1,OPEN CODE)",
                        list {
                            { "0", std::make_shared<test_var_value>("MAC_1") },
                            { "1", std::make_shared<test_var_value>("OPEN CODE") },
                        }),
                },
            },
            {} // empty ord symbols
            ));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_into(d, m, exp_frames, 3, "MACRO", file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(),
        frame_vars_ignore_sys_vars({},
            std::unordered_map<std::string, test_var_value> {
                // macro locals
                {
                    "&SYSLIST",
                    test_var_value("(A,())",
                        list {
                            { "0", std::make_shared<test_var_value>("A") },
                            { "1",
                                std::make_shared<test_var_value>("()",
                                    list {
                                        { "1", std::make_shared<test_var_value>("") },
                                    }) },
                        }),
                },
                {

                    "&SYSMAC",
                    test_var_value("(MAC_IN,MAC_1,OPEN CODE)",
                        list {
                            { "0", std::make_shared<test_var_value>("MAC_IN") },
                            { "1", std::make_shared<test_var_value>("MAC_1") },
                            { "2", std::make_shared<test_var_value>("OPEN CODE") },
                        }),
                },
            },
            {} // empty ord symbols
            ));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 4);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_into(d, m, exp_frames, 1, "COPY", copy1_file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(), exp_frame_vars[0]);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    erase_frames_from_top(3, exp_frames, exp_frame_vars);
    step_over_by(3, d, m, exp_frames, 17);
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_into(d, m, exp_frames, 14, "MACRO", file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(),
        frame_vars_ignore_sys_vars({},
            std::unordered_map<std::string, test_var_value> {
                // macro locals
                {
                    "&SYSLIST",
                    test_var_value("(,3456,(),,((1,('a','b')),2),((())))",
                        list {
                            { "0", std::make_shared<test_var_value>("") },
                            { "1", std::make_shared<test_var_value>("3456") },
                            { "2",
                                std::make_shared<test_var_value>("()",
                                    list {
                                        { "1", std::make_shared<test_var_value>("") },
                                    }) },
                            { "3", std::make_shared<test_var_value>("") },
                            { "4",
                                std::make_shared<test_var_value>("((1,('a','b')),2)",
                                    list {
                                        { "1",
                                            std::make_shared<test_var_value>("(1,('a','b'))",
                                                list {
                                                    { "1", std::make_shared<test_var_value>("1") },
                                                    { "2",
                                                        std::make_shared<test_var_value>("('a','b')",
                                                            list {
                                                                { "1", std::make_shared<test_var_value>("'a'") },
                                                                { "2", std::make_shared<test_var_value>("'b'") },
                                                            }) },
                                                }) },
                                        { "2", std::make_shared<test_var_value>("2") },

                                    }) },
                            { "5",
                                std::make_shared<test_var_value>("((()))",
                                    list {
                                        { "1",
                                            std::make_shared<test_var_value>("(())",
                                                list {
                                                    { "1",
                                                        std::make_shared<test_var_value>("()",
                                                            list {
                                                                { "1", std::make_shared<test_var_value>("") },
                                                            }) },
                                                }) },
                                    }) },
                        }),
                },
                {
                    "&SYSMAC",
                    test_var_value("(MAC_2,OPEN CODE)",
                        list {
                            { "0", std::make_shared<test_var_value>("MAC_2") },
                            { "1", std::make_shared<test_var_value>("OPEN CODE") },
                        }),
                },
            },
            {} // empty ord symbols
            ));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.disconnect();
}

TEST(debugger, positional_parameters)
{
    using list = std::unordered_map<std::string, std::shared_ptr<test_var_value>>;

    std::string open_code = R"(
        MACRO
        MAC &VAR
        MEND

        MAC
        MAC ()
        MAC (13,14)
        MAC (('a','b'),('c'),('d'))
)";

    file_manager_impl file_manager;
    workspace_mock lib_provider(file_manager);
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration)
        .WillRepeatedly(Invoke([&file_manager, &lib_provider](auto uri, auto r) {
            resource_location res = resource_location(std::string_view(uri));
            r.provide({
                .fm = &file_manager,
                .libraries = lib_provider.get_libraries(res),
                .workspace_uri = resource_location(lib_provider.uri()),
                .opts = lib_provider.get_asm_options(res),
                .pp_opts = lib_provider.get_preprocessor_options(res),
            });
        }));
    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string filename = "ws\\test";
    resource_location file_loc(filename);

    file_manager.did_open_file(file_loc, 0, open_code);

    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(filename, dc_provider, true, resp);
    m.wait_for_stopped();

    d.next();
    m.wait_for_stopped();
    std::vector<expected_stack_frame> exp_frames { { 5, 5, 0, "OPENCODE", file_loc.get_uri() } };
    std::vector<frame_vars> exp_frame_vars { { {}, {}, {} } };

    step_into(d, m, exp_frames, 3, "MACRO", file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(),
        frame_vars_ignore_sys_vars({}, // empty globals
            std::unordered_map<std::string, test_var_value> {
                // macro locals
                { "&VAR", "" },
            },
            {} // empty ord symbols
            ));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    erase_frames_from_top(1, exp_frames, exp_frame_vars);
    step_over_by(1, d, m, exp_frames, 6);

    step_into(d, m, exp_frames, 3, "MACRO", file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(),
        frame_vars_ignore_sys_vars({}, // empty globals
            std::unordered_map<std::string, test_var_value> {
                // macro locals
                {
                    "&VAR",
                    test_var_value("()",
                        list {
                            { "1", std::make_shared<test_var_value>("") },
                        }),
                },
            },
            {} // empty ord symbols
            ));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    erase_frames_from_top(1, exp_frames, exp_frame_vars);
    step_over_by(1, d, m, exp_frames, 7);

    step_into(d, m, exp_frames, 3, "MACRO", file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(),
        frame_vars_ignore_sys_vars({}, // empty globals
            std::unordered_map<std::string, test_var_value> {
                // macro locals
                {
                    "&VAR",
                    test_var_value("(13,14)",
                        list {
                            { "1", std::make_shared<test_var_value>("13") },
                            { "2", std::make_shared<test_var_value>("14") },
                        }),
                },
            },
            {} // empty ord symbols
            ));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    erase_frames_from_top(1, exp_frames, exp_frame_vars);
    step_over_by(1, d, m, exp_frames, 8);

    step_into(d, m, exp_frames, 3, "MACRO", file_loc);
    exp_frame_vars.insert(exp_frame_vars.begin(),
        frame_vars_ignore_sys_vars({}, // empty globals
            std::unordered_map<std::string, test_var_value> {
                // macro locals
                {
                    "&VAR",
                    test_var_value("(('a','b'),('c'),('d'))",
                        list {
                            { "1",
                                std::make_shared<test_var_value>("('a','b')",
                                    list {
                                        { "1", std::make_shared<test_var_value>("'a'") },
                                        { "2", std::make_shared<test_var_value>("'b'") },
                                    }) },
                            { "2",
                                std::make_shared<test_var_value>("('c')",
                                    list {
                                        { "1", std::make_shared<test_var_value>("'c'") },
                                    }) },
                            { "3",
                                std::make_shared<test_var_value>("('d')",
                                    list {
                                        { "1", std::make_shared<test_var_value>("'d'") },
                                    }) },
                        }),
                },
            },
            {} // empty ord symbols
            ));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.disconnect();
}

TEST(debugger, arrays)
{
    using list = std::unordered_map<std::string, std::shared_ptr<test_var_value>>;

    std::string open_code = R"(
&VAR(30)  SETA 1,456,48,7
&BOOL(15) SETB 0,1,0
&STR(6)   SETC 'a','b'
          GBLC &ARR(10)
          END
)";


    file_manager_impl file_manager;
    workspace_mock lib_provider(file_manager);
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration)
        .WillRepeatedly(Invoke([&file_manager, &lib_provider](auto uri, auto r) {
            resource_location res = resource_location(std::string_view(uri));
            r.provide({
                .fm = &file_manager,
                .libraries = lib_provider.get_libraries(res),
                .workspace_uri = resource_location(lib_provider.uri()),
                .opts = lib_provider.get_asm_options(res),
                .pp_opts = lib_provider.get_preprocessor_options(res),
            });
        }));
    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string filename = "ws\\test";
    resource_location file_loc(filename);

    file_manager.did_open_file(file_loc, 0, open_code);

    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(filename, dc_provider, true, resp);
    m.wait_for_stopped();
    std::vector<expected_stack_frame> exp_frames { { 1, 1, 0, "OPENCODE", file_loc.get_uri() } };
    std::vector<frame_vars> exp_frame_vars { { {}, {}, {} } };
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 2);
    exp_frame_vars[0].locals.emplace("&VAR",
        test_var_value("(1,456,48,7)",
            list { { "30", std::make_shared<test_var_value>(1) },
                { "31", std::make_shared<test_var_value>(456) },
                { "32", std::make_shared<test_var_value>(48) },
                { "33", std::make_shared<test_var_value>(7) } }));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 3);
    exp_frame_vars[0].locals.emplace("&BOOL",
        test_var_value("(FALSE,TRUE,FALSE)",
            list { { "15", std::make_shared<test_var_value>("FALSE") },
                { "16", std::make_shared<test_var_value>("TRUE") },
                { "17", std::make_shared<test_var_value>("FALSE") } }));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 4);
    exp_frame_vars[0].locals.emplace("&STR",
        test_var_value("(a,b)",
            list { { "6", std::make_shared<test_var_value>("a") }, { "7", std::make_shared<test_var_value>("b") } }));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 5);
    exp_frame_vars[0].globals.emplace("&ARR", test_var_value("()", list {}));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.disconnect();
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
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration)
        .WillRepeatedly(Invoke([&file_manager, &lib_provider](auto uri, auto r) {
            resource_location res = resource_location(std::string_view(uri));
            r.provide({
                .fm = &file_manager,
                .libraries = lib_provider.get_libraries(res),
                .workspace_uri = resource_location(lib_provider.uri()),
                .opts = lib_provider.get_asm_options(res),
                .pp_opts = lib_provider.get_preprocessor_options(res),
            });
        }));
    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string filename = "ws\\test";
    resource_location file_loc(filename);

    file_manager.did_open_file(file_loc, 0, open_code);

    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(file_loc.get_uri(), dc_provider, true, resp);

    m.wait_for_stopped();
    std::vector<expected_stack_frame> exp_frames { { 1, 1, 0, "OPENCODE", file_loc.get_uri() } };
    std::vector<frame_vars> exp_frame_vars { { {}, {}, {} } };
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_over_by(1, d, m, exp_frames, 2);
    exp_frame_vars[0].ord_syms.emplace("A",
        test_var_value("13",
            list { { "L", std::make_shared<test_var_value>("1") }, { "T", std::make_shared<test_var_value>("U") } }));
    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    d.disconnect();
}

TEST(debugger, ainsert)
{
    using list = std::unordered_map<std::string, std::shared_ptr<test_var_value>>;

    std::string open_code = R"(
    MACRO
    MAC
    AINSERT '       MACRO',BACK
    AINSERT '       MAC_GEN',BACK
    AINSERT '       GBLA &&VAR',BACK
    AINSERT '&&VAR  SETA 5',BACK
    AINSERT '       MEND',BACK
    MEND
    
    GBLA &VAR
    MAC
    MAC_GEN
    END
)";


    file_manager_impl file_manager;
    workspace_mock lib_provider(file_manager);
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration)
        .WillRepeatedly(Invoke([&file_manager, &lib_provider](auto uri, auto r) {
            resource_location res = resource_location(std::string_view(uri));
            r.provide({
                .fm = &file_manager,
                .libraries = lib_provider.get_libraries(res),
                .workspace_uri = resource_location(lib_provider.uri()),
                .opts = lib_provider.get_asm_options(res),
                .pp_opts = lib_provider.get_preprocessor_options(res),
            });
        }));
    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string filename = "ws\\test";
    resource_location file_loc(filename);

    file_manager.did_open_file(file_loc, 0, open_code);

    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(file_loc.get_uri(), dc_provider, true, resp);

    m.wait_for_stopped();
    std::vector<expected_stack_frame> exp_frames { { 1, 1, 0, "OPENCODE", file_loc.get_uri() } };
    std::vector<frame_vars> exp_frame_vars { {
        std::unordered_map<std::string, test_var_value> {
            // macro locals
            {
                "&VAR",
                0,
            },
        },
        {}, // empty locals
        {} // empty ord symbols
    } };

    step_over_by(3, d, m, exp_frames, 12);
    step_into(d, m, exp_frames, 2, "MACRO", [](std::string_view uri) {
        static const std::regex expected("hlasm://\\d+/AINSERT_1.hlasm");
        return std::regex_match(std::string(uri), expected);
    });
    exp_frame_vars.insert(exp_frame_vars.begin(), frame_vars_ignore_sys_vars({}, {}, {}));

    EXPECT_TRUE(check_step(d, exp_frames, exp_frame_vars));

    step_out(d, m, exp_frames, 13);

    exp_frame_vars.erase(exp_frame_vars.begin());
    exp_frame_vars.front().globals.at("&VAR") = test_var_value(5);

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
    resource_location copy1_file_loc(copy1_filename);
    std::string copy1_source = R"(
        LR 1,1
        LR 1,1
        LR 1,1
        LR 1,1
        LR 1,1
        LR 1,1
)";


    file_manager_impl file_manager;
    file_manager.did_open_file(copy1_file_loc, 0, copy1_source);
    workspace_mock lib_provider(file_manager);
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration)
        .WillRepeatedly(Invoke([&file_manager, &lib_provider](auto uri, auto r) {
            resource_location res = resource_location(std::string_view(uri));
            r.provide({
                .fm = &file_manager,
                .libraries = lib_provider.get_libraries(res),
                .workspace_uri = resource_location(lib_provider.uri()),
                .opts = lib_provider.get_asm_options(res),
                .pp_opts = lib_provider.get_preprocessor_options(res),
            });
        }));

    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string filename = "ws\\test";
    resource_location file_loc(filename);

    file_manager.did_open_file(file_loc, 0, open_code);
    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(filename, dc_provider, true, resp);
    m.wait_for_stopped();
    std::string new_string = "SOME NEW FILE DOES NOT MATTER";
    std::vector<document_change> chs;
    chs.emplace_back(new_string.c_str(), new_string.size());
    d.next();
    std::thread t([&file_manager, &copy1_file_loc, &chs]() {
        file_manager.did_change_file(copy1_file_loc, 0, chs.data(), chs.size());
    });
    t.join();
    m.wait_for_stopped();

    d.disconnect();
}

TEST(debugger, pimpl_moves)
{
    debugger d;
    debugger d2;

    d2 = std::move(d);
}

TEST(debugger, breakpoints_set_get)
{
    debugger d;

    breakpoint bp(5);

    d.breakpoints("file", sequence<breakpoint>(&bp, 1));
    auto bps = d.breakpoints("file");

    ASSERT_EQ(bps.size(), 1);
    EXPECT_EQ(bp.line, bps.begin()->line);
}

TEST(debugger, function_breakpoints)
{
    std::string open_code = R"(
    LR 1,1
    SAM31
)";

    file_manager_impl file_manager;
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration).WillRepeatedly(Invoke([&file_manager](auto, auto r) {
        r.provide({ .fm = &file_manager });
    }));
    debugger d;
    debug_event_consumer_s_mock m(d);

    const resource_location file_loc("test");

    file_manager.did_open_file(file_loc, 0, open_code);

    function_breakpoint bp(sequence(std::string_view("SAM31")));
    d.function_breakpoints(sequence<function_breakpoint>(&bp, 1));

    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(file_loc.get_uri(), dc_provider, false, resp);

    m.wait_for_stopped();

    EXPECT_EQ(m.get_last_reason(), "function breakpoint");

    d.disconnect();
}

TEST(debugger, invalid_file)
{
    file_manager_impl file_manager;
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration).WillRepeatedly(Invoke([&file_manager](auto, auto r) {
        r.provide({ .fm = &file_manager });
    }));

    debugger d;
    debug_event_consumer_s_mock m(d);
    std::string file_name = "test_workspace\\test";
    resource_location file_loc(file_name);

    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(false));
    d.launch(file_name.c_str(), dc_provider, true, resp);

    while (!resp.resolved())
        d.analysis_step(nullptr);

    d.disconnect();
}

TEST(debugger, evaluate)
{
    std::string open_code = R"(
    MACRO
    INNER
&A  SETA 123
&B  SETB 1
&C  SETC 'ABCDEF'
    MEND
*
    MACRO
    OUTER
    INNER &SYSLIST(1),&SYSLIST(2),&SYSLIST(3)
    MEND
*
C   CSECT
    DS    F
L   DS    F
*
    OUTER X,Y,Z
)";


    file_manager_impl file_manager;
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration).WillRepeatedly(Invoke([&file_manager](auto, auto r) {
        r.provide({ .fm = &file_manager });
    }));
    debugger d;
    debug_event_consumer_s_mock m(d);

    const resource_location file_loc("test");

    file_manager.did_open_file(file_loc, 0, open_code);

    breakpoint bp(6);
    d.breakpoints(file_loc.get_uri(), sequence<breakpoint>(&bp, 1));

    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(file_loc.get_uri(), dc_provider, false, resp);

    m.wait_for_stopped();

    std::tuple<std::tuple<std::string_view, frame_id_t>, std::tuple<std::string_view, bool, bool>> cases[] = {
        { { "&SYSNEST", -1 }, { "2", false, false } },
        { { "&SYSNEST", 2 }, { "2", false, false } },
        { { "&SYSNEST", 1 }, { "1", false, false } },
        { { "&SYSNEST", 0 }, { "", true, false } },
        { { "&A", 2 }, { "123", false, false } },
        { { "&B", 2 }, { "TRUE", false, false } },
        { { "&C", 2 }, { "ABCDEF", false, false } },
        { { "&A", 1 }, { "", true, false } },
        { { "&B", 1 }, { "", true, false } },
        { { "&C", 1 }, { "", true, false } },
        { { "&A+&A", -1 }, { "246", false, false } },
        { { "(&B)", -1 }, { "TRUE", false, false } },
        { { "(&A GE 100)", -1 }, { "TRUE", false, false } },
        { { "(&A LT 100)", -1 }, { "FALSE", false, false } },
        { { "'&C'", -1 }, { "ABCDEF", false, false } },
        { { "'&C'(3,2)", -1 }, { "CD", false, false } },
        { { "'&SYSLIST(&SYSNEST)'", 2 }, { "Y", false, false } },
        { { "'&SYSLIST(&SYSNEST)'", 1 }, { "X", false, false } },
        { { "'&SYSLIST(&SYSNEST)'", 0 }, { "", true, false } },
        { { "&SYSLIST", -1 }, { "(,X,Y,Z)", false, true } },
        { { "L", -1 }, { "C + X'4'", false, false } },
        { { "L", 0 }, { "C + X'4'", false, false } },
        { { "L", 1 }, { "C + X'4'", false, false } },
        { { "L", 2 }, { "C + X'4'", false, false } },
        { { "K'&C", 2 }, { "6", false, false } },
        { { "&A + &A", -1 }, { "", true, false } },
    };

    for (const auto& [args, expected] : cases)
    {
        const auto& [expr, frame] = args;
        SCOPED_TRACE(testing::Message() << expr << ", " << frame);

        const auto& [exp, error, structured] = expected;
        auto res = d.evaluate(sequence<char>(expr), frame);

        if (!exp.empty())
            EXPECT_EQ(std::string_view(res.result()), exp);
        EXPECT_EQ(res.is_error(), error);
        EXPECT_EQ(!!res.var_ref(), structured);
    }

    d.disconnect();
}

TEST(debugger, outputs)
{
    std::string open_code = R"(
    MNOTE 1,'mnote test'
    PUNCH 'punch test'
)";


    file_manager_impl file_manager;
    NiceMock<debugger_configuration_provider_mock> dc_provider;
    EXPECT_CALL(dc_provider, provide_debugger_configuration).WillRepeatedly(Invoke([&file_manager](auto, auto r) {
        r.provide({ .fm = &file_manager });
    }));
    debugger d;
    debug_event_consumer_s_mock m(d);

    const resource_location file_loc("test");

    file_manager.did_open_file(file_loc, 0, open_code);

    auto [resp, mock] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<bool>>);
    EXPECT_CALL(*mock, provide(true));
    d.launch(file_loc.get_uri(), dc_provider, false, resp);

    m.wait_for_exited();

    d.disconnect();

    const std::pair<unsigned char, std::string> expected_mnote((unsigned char)1, "mnote test");
    const std::string_view expected_punch = "punch test";
    EXPECT_EQ(m.get_last_mnote(), expected_mnote);
    EXPECT_EQ(m.get_last_punch(), expected_punch);
}
