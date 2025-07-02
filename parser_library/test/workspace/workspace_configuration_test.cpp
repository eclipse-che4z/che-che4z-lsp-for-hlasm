/*
 * Copyright (c) 2022 Broadcom.
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
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "empty_configs.h"
#include "external_configuration_requests.h"
#include "external_configuration_requests_mock.h"
#include "file_manager_mock.h"
#include "nlohmann/json.hpp"
#include "utils/content_loader.h"
#include "utils/platform.h"
#include "utils/resource_location.h"
#include "utils/task.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/library_local.h"
#include "workspaces/workspace_configuration.h"

using namespace ::testing;
using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils;
using namespace hlasm_plugin::utils::resource;

namespace {
template<int>
struct X
{
    char a;

    auto operator<=>(const X&) const = default;
};
} // namespace

TEST(workspace_configuration, library_options)
{
    static_assert(!std::is_copy_constructible_v<library_options>);

    constexpr auto eq = [](auto&& l, auto&& r) { return !(l < r || r < l); };

    X<0> x0_0 { 0 };
    library_options lx0_1(X<0> { 0 });
    EXPECT_TRUE(eq(lx0_1, x0_0));

    library_options lx0_2(X<0> { 0 });
    EXPECT_TRUE(eq(lx0_1, lx0_2));

    library_options lx0_moved_1(std::move(lx0_1));
    library_options lx0_moved_2(std::move(lx0_2));
    EXPECT_TRUE(eq(lx0_moved_1, lx0_moved_2));
    EXPECT_TRUE(eq(lx0_1, lx0_2));
    EXPECT_FALSE(eq(lx0_1, lx0_moved_1));

    X<0> x0_1 { 1 };
    EXPECT_TRUE(lx0_moved_1 < x0_1);
    EXPECT_TRUE(lx0_moved_1 < library_options(x0_1));

    X<1> x1_0 { 0 };
    EXPECT_FALSE(eq(lx0_moved_1, x1_0));
    EXPECT_FALSE(eq(lx0_moved_1, library_options(x1_0)));

    library_options lx1_1(x1_0);
    library_options lx1_2(x1_0);
    library_options lx1_3(x0_0);

    EXPECT_TRUE(eq(lx1_1, lx1_2));
    EXPECT_FALSE(eq(lx1_2, lx1_3));

    lx1_3 = library_options(x1_0);
    EXPECT_TRUE(eq(lx1_1, lx1_3));

    EXPECT_TRUE(library_options(X<2> { 1 }) < X<2> { 2 });
    EXPECT_TRUE(library_options(X<2> { 1 }) < library_options(X<2> { 2 }));
}

TEST(workspace_configuration, refresh_needed_configs)
{
    NiceMock<file_manager_mock> fm;
    shared_json global_settings = make_empty_shared_json();
    lib_config global_config;

    EXPECT_CALL(fm, get_file_content(_))
        .WillRepeatedly(Invoke([](const auto&) -> hlasm_plugin::utils::value_task<std::optional<std::string>> {
            co_return std::nullopt;
        }));

    workspace_configuration cfg(
        fm, resource_location("test://workspace"), global_settings, global_config, nullptr, nullptr);

    EXPECT_TRUE(cfg.refresh_libraries({ resource_location("test://workspace/.hlasmplugin") }).run().value());
    EXPECT_TRUE(
        cfg.refresh_libraries({ resource_location("test://workspace/.hlasmplugin/proc_grps.json") }).run().value());
    EXPECT_TRUE(
        cfg.refresh_libraries({ resource_location("test://workspace/.hlasmplugin/pgm_conf.json") }).run().value());
    EXPECT_FALSE(cfg.refresh_libraries({ resource_location("test://workspace/something/else") }).run().value());
}

TEST(workspace_configuration, external_configurations_group_name)
{
    NiceMock<file_manager_mock> fm;
    shared_json global_settings = make_empty_shared_json();
    lib_config global_config;
    NiceMock<external_configuration_requests_mock> ext_confg;

    EXPECT_CALL(fm, get_file_content(resource_location("test://workspace/.hlasmplugin/proc_grps.json")))
        .WillOnce(Invoke([]() {
            return value_task<std::optional<std::string>>::from_value(R"(
{
  "pgroups": [
    {
      "name": "GRP1",
      "libs": [],
      "asm_options": {"SYSPARM": "PARM1"}
    }
  ]
}
)");
        }));
    EXPECT_CALL(fm, get_file_content(resource_location("test://workspace/.hlasmplugin/pgm_conf.json")))
        .WillOnce(Invoke([]() { return value_task<std::optional<std::string>>::from_value(std::nullopt); }));

    workspace_configuration cfg(
        fm, resource_location("test://workspace"), global_settings, global_config, &ext_confg, nullptr);
    cfg.parse_configuration_file().run();

    EXPECT_CALL(ext_confg,
        read_external_configuration(Truly([](std::string_view v) { return v == "test://workspace/file1.hlasm"; }), _))
        .WillOnce(Invoke([](auto, auto channel) { channel.provide(R"("GRP1")"); }));

    const resource_location pgm_loc("test://workspace/file1.hlasm");

    cfg.load_alternative_config_if_needed(pgm_loc).run();

    const auto* pgm = cfg.get_program(pgm_loc);

    ASSERT_TRUE(pgm);
    EXPECT_TRUE(pgm->external);
    EXPECT_EQ(pgm->pgroup, proc_grp_id(basic_conf { "GRP1" }));

    const auto& grp = cfg.get_proc_grp(pgm->pgroup);

    asm_option opts;
    grp.apply_options_to(opts);
    EXPECT_EQ(opts, asm_option { .sysparm = "PARM1" });
}

TEST(workspace_configuration, external_configurations_group_inline)
{
    NiceMock<file_manager_mock> fm;
    shared_json global_settings = make_empty_shared_json();
    lib_config global_config;
    NiceMock<external_configuration_requests_mock> ext_confg;

    EXPECT_CALL(fm, get_file_content(_)).WillRepeatedly(Invoke([]() {
        return value_task<std::optional<std::string>>::from_value(std::nullopt);
    }));

    workspace_configuration cfg(
        fm, resource_location("test://workspace"), global_settings, global_config, &ext_confg, nullptr);
    cfg.parse_configuration_file().run();

    EXPECT_CALL(ext_confg,
        read_external_configuration(Truly([](std::string_view v) { return v == "test://workspace/file1.hlasm"; }), _))
        .WillOnce(Invoke([](auto, auto channel) {
            channel.provide(R"({
      "name": "GRP1",
      "libs": [
        "path"
      ],
      "asm_options": {"SYSPARM": "PARM1"}
    })");
        }));

    const resource_location pgm_loc("test://workspace/file1.hlasm");

    cfg.load_alternative_config_if_needed(pgm_loc).run();

    const auto* pgm = cfg.get_program(pgm_loc);
    ASSERT_TRUE(pgm);
    EXPECT_TRUE(pgm->external);
    EXPECT_TRUE(std::holds_alternative<external_conf>(pgm->pgroup));

    const auto& grp = cfg.get_proc_grp(pgm->pgroup);

    asm_option opts;
    grp.apply_options_to(opts);
    EXPECT_EQ(opts, asm_option { .sysparm = "PARM1" });
    EXPECT_EQ(grp.libraries().size(), 1);
}

TEST(workspace_configuration, external_configurations_prune)
{
    NiceMock<file_manager_mock> fm;
    shared_json global_settings = make_empty_shared_json();
    lib_config global_config;
    NiceMock<external_configuration_requests_mock> ext_confg;

    EXPECT_CALL(fm, get_file_content(_)).WillRepeatedly(Invoke([]() {
        return value_task<std::optional<std::string>>::from_value(std::nullopt);
    }));

    workspace_configuration cfg(
        fm, resource_location("test://workspace"), global_settings, global_config, &ext_confg, nullptr);
    cfg.parse_configuration_file().run();

    static constexpr std::string_view grp_def(R"({
      "name": "GRP1",
      "libs": [
        "path"
      ],
      "asm_options": {"SYSPARM": "PARM1"}
    })");

    EXPECT_CALL(ext_confg,
        read_external_configuration(Truly([](std::string_view v) { return v == "test://workspace/file1.hlasm"; }), _))
        .WillOnce(Invoke([](auto, auto channel) { channel.provide(grp_def); }));

    const resource_location pgm_loc("test://workspace/file1.hlasm");

    cfg.load_alternative_config_if_needed(pgm_loc).run();

    EXPECT_CALL(ext_confg,
        read_external_configuration(Truly([](std::string_view v) { return v == "test://workspace/file2.hlasm"; }), _))
        .WillOnce(Invoke([](auto, auto channel) { channel.provide(grp_def); }));

    const resource_location pgm_loc_2("test://workspace/file2.hlasm");

    cfg.load_alternative_config_if_needed(pgm_loc_2).run();

    cfg.prune_external_processor_groups(pgm_loc);

    EXPECT_EQ(cfg.get_program(pgm_loc), nullptr);
    EXPECT_NE(cfg.get_program(pgm_loc_2), nullptr);

    EXPECT_NO_THROW(cfg.get_proc_grp(external_conf { std::make_shared<std::string>(grp_def) }));

    cfg.prune_external_processor_groups(pgm_loc_2);

    EXPECT_EQ(cfg.get_program(pgm_loc), nullptr);
    EXPECT_EQ(cfg.get_program(pgm_loc_2), nullptr);

    EXPECT_THROW(cfg.get_proc_grp(external_conf { std::make_shared<std::string>(grp_def) }), std::out_of_range);
}

TEST(workspace_configuration, external_configurations_prune_all)
{
    NiceMock<file_manager_mock> fm;
    shared_json global_settings = make_empty_shared_json();
    lib_config global_config;
    NiceMock<external_configuration_requests_mock> ext_confg;

    EXPECT_CALL(fm, get_file_content(_)).WillRepeatedly(Invoke([]() {
        return value_task<std::optional<std::string>>::from_value(std::nullopt);
    }));

    workspace_configuration cfg(
        fm, resource_location("test://workspace"), global_settings, global_config, &ext_confg, nullptr);
    cfg.parse_configuration_file().run();

    static constexpr std::string_view grp_def(R"({
      "name": "GRP1",
      "libs": [
        "path"
      ],
      "asm_options": {"SYSPARM": "PARM1"}
    })");

    EXPECT_CALL(ext_confg,
        read_external_configuration(Truly([](std::string_view v) { return v == "test://workspace/file1.hlasm"; }), _))
        .WillOnce(Invoke([](auto, auto channel) { channel.provide(grp_def); }));

    const resource_location pgm_loc("test://workspace/file1.hlasm");

    cfg.load_alternative_config_if_needed(pgm_loc).run();

    cfg.prune_external_processor_groups(resource_location());

    const auto* pgm = cfg.get_program(pgm_loc);

    EXPECT_EQ(pgm, nullptr);

    EXPECT_THROW(cfg.get_proc_grp(external_conf { std::make_shared<std::string>(grp_def) }), std::out_of_range);
}

TEST(workspace_configuration, refresh_settings)
{
    file_manager_impl fm;

    fm.did_open_file(empty_pgm_conf_name,
        0,
        R"({"pgms":[{"program":"test/${config:pgm_mask.0}","pgroup":"P1","asm_options":{"SYSPARM":"${config:sysparm}${config:sysparm}"}}]})");
    fm.did_open_file(empty_proc_grps_name, 0, R"({"pgroups":[{"name": "P1","libs":[]}]})");

    lib_config config;
    shared_json global_settings = std::make_shared<const nlohmann::json>(
        nlohmann::json::parse(R"({"pgm_mask":["file_name"],"sysparm":"DEBUG"})"));
    lib_config global_config;
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, global_config, nullptr, nullptr);
    const auto test_loc = resource_location::join(empty_ws, "test");
    ws_cfg.parse_configuration_file().run();

    std::vector<diagnostic> diags;
    ws_cfg.produce_diagnostics(diags, {}, {});
    EXPECT_TRUE(diags.empty());

    using hlasm_plugin::utils::resource::resource_location;

    auto file_name = resource_location::join(test_loc, "file_name");
    auto different_file = resource_location::join(test_loc, "different_file");

    EXPECT_EQ(ws_cfg.get_analyzer_configuration(file_name).run().value().first.opts.sysparm, "DEBUGDEBUG");
    EXPECT_FALSE(ws_cfg.settings_updated());

    global_settings = std::make_shared<const nlohmann::json>(
        nlohmann::json::parse(R"({"pgm_mask":["different_file"],"sysparm":"RELEASE"})"));
    EXPECT_TRUE(ws_cfg.settings_updated());
    EXPECT_EQ(ws_cfg.parse_configuration_file().run().value(), parse_config_file_result::parsed);

    EXPECT_EQ(ws_cfg.get_analyzer_configuration(file_name).run().value().first.opts.sysparm, "");
    EXPECT_EQ(ws_cfg.get_analyzer_configuration(different_file).run().value().first.opts.sysparm, "RELEASERELEASE");
}

using hlasm_plugin::utils::platform::is_windows;
using namespace hlasm_plugin::utils::resource;

const auto users_dir =
    is_windows() ? resource_location("file:///c%3A/Users/") : resource_location("file:///home/user/");
const auto ws_loc = resource_location::join(users_dir, "ws/");
const auto pgm_override_loc = resource_location::join(ws_loc, "pgm_override");
const auto pgm_anything_loc = resource_location::join(ws_loc, "pgms/anything");
const auto pgm_outside_ws = resource_location::join(users_dir, "outside/anything");
const auto pgm1_loc = resource_location::join(ws_loc, "pgm1");
const std::string file_proc_grps_content = is_windows() ?
                                                        R"({
    "pgroups": [
        {
            "name": "P1",
            "libs": [
                "C:\\Users\\Desktop\\ASLib",
                "lib",
                "libs\\lib2\\",
                "file:///c%3A/Users/Desktop/Temp/",
                ""
            ],
            "asm_options": {
                "SYSPARM": "SEVEN",
                "PROFILE": "MAC1"
            },
            "preprocessor": "DB2"
        },
        {
            "name": "P2",
            "libs": [
                "C:\\Users\\Desktop\\ASLib",
                "P2lib",
                "P2libs\\libb"
            ]
        }
    ]
})"
                                                        : R"({
    "pgroups": [
        {
            "name": "P1",
            "libs": [
                "/home/user/ASLib",
                "lib",
                "libs/lib2/",
                "file:///home/user/Temp/",
                ""
            ],
            "asm_options": {
                "SYSPARM": "SEVEN",
                "PROFILE": "MAC1"
            },
            "preprocessor": "DB2"
        },
        {
            "name": "P2",
            "libs": [
                "/home/user/ASLib",
                "P2lib",
                "P2libs/libb"
            ]
        }
    ]
})";

const std::string file_pgm_conf_content = is_windows() ? R"({
  "pgms": [
    {
      "program": "pgm1",
      "pgroup": "P1"
    },
    {
      "program": "pgm_override",
      "pgroup": "P1",
      "asm_options":
      {
        "PROFILE": "PROFILE OVERRIDE"
      }
    },
    {
      "program": "pgms\\*",
      "pgroup": "P2"
    }
  ]
})"
                                                       : R"({
  "pgms": [
    {
      "program": "pgm1",
      "pgroup": "P1"
    },
    {
      "program": "pgm_override",
      "pgroup": "P1",
      "asm_options":
      {
        "PROFILE": "PROFILE OVERRIDE"
      }
    },
    {
      "program": "pgms/*",
      "pgroup": "P2"
    }
  ]
})";



class file_manager_proc_grps_test : public file_manager_impl
{
public:
    hlasm_plugin::utils::value_task<std::optional<std::string>> get_file_content(
        const resource_location& location) override
    {
        using hlasm_plugin::utils::value_task;
        if (hlasm_plugin::utils::resource::filename(location) == "proc_grps.json")
            return value_task<std::optional<std::string>>::from_value(file_proc_grps_content);
        else if (hlasm_plugin::utils::resource::filename(location) == "pgm_conf.json")
            return value_task<std::optional<std::string>>::from_value(file_pgm_conf_content);
        else
            return value_task<std::optional<std::string>>::from_value(std::nullopt);
    }

    // Inherited via file_manager
    file_content_state did_open_file(const resource_location&, version_t, std::string) override
    {
        return file_content_state::changed_content;
    }
    void did_change_file(const resource_location&, version_t, std::span<const document_change>) override {}
    void did_close_file(const resource_location&) override {}
};

void check_process_group(const processor_group& pg, std::span<resource_location> expected)
{
    EXPECT_EQ(std::size(expected), pg.libraries().size()) << "For pg.name() = " << pg.name();
    for (size_t i = 0; i < std::min(std::size(expected), pg.libraries().size()); ++i)
    {
        library_local* libl = dynamic_cast<library_local*>(pg.libraries()[i].get());
        ASSERT_NE(libl, nullptr);
        EXPECT_EQ(expected[i], libl->get_location())
            << "Expected: " << expected[i].get_uri() << "\n Got: " << libl->get_location().get_uri()
            << "\n For pg.name() = " << pg.name() << " and i = " << i;
    }
}

TEST(workspace_configuration, load_config_synthetic)
{
    file_manager_proc_grps_test file_manager;
    shared_json global_settings = make_empty_shared_json();
    lib_config global_config;
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, global_config, nullptr, nullptr);

    ws_cfg.parse_configuration_file().run();

    // Check P1
    auto& pg = ws_cfg.get_proc_grp(basic_conf { "P1" });
    EXPECT_EQ("P1", pg.name());
    auto expected = []() -> std::array<resource_location, 5> {
        if (is_windows())
            return { resource_location("file:///c%3A/Users/Desktop/ASLib/"),
                resource_location("file:///c%3A/Users/ws/lib/"),
                resource_location("file:///c%3A/Users/ws/libs/lib2/"),
                resource_location("file:///c%3A/Users/Desktop/Temp/"),
                resource_location("file:///c%3A/Users/ws/") };
        else
            return { resource_location("file:///home/user/ASLib/"),
                resource_location("file:///home/user/ws/lib/"),
                resource_location("file:///home/user/ws/libs/lib2/"),
                resource_location("file:///home/user/Temp/"),
                resource_location("file:///home/user/ws/") };
    }();
    check_process_group(pg, expected);

    // Check P2
    auto& pg2 = ws_cfg.get_proc_grp(basic_conf { "P2" });
    EXPECT_EQ("P2", pg2.name());

    auto expected2 = []() -> std::array<resource_location, 3> {
        if (is_windows())
            return { resource_location("file:///c%3A/Users/Desktop/ASLib/"),
                resource_location("file:///c%3A/Users/ws/P2lib/"),
                resource_location("file:///c%3A/Users/ws/P2libs/libb/") };
        else
            return { resource_location("file:///home/user/ASLib/"),
                resource_location("file:///home/user/ws/P2lib/"),
                resource_location("file:///home/user/ws/P2libs/libb/") };
    }();
    check_process_group(pg2, expected2);

    // Check PGM1
    // test of pgm_conf and workspace::get_proc_grp
    const auto* pg3 = ws_cfg.get_proc_grp(pgm1_loc);
    ASSERT_TRUE(pg3);
    check_process_group(*pg3, expected);

    // Check PGM anything
    const auto* pg4 = ws_cfg.get_proc_grp(pgm_anything_loc);
    ASSERT_TRUE(pg4);
    check_process_group(*pg4, expected2);

    auto [analyzer_opts, _] = ws_cfg.get_analyzer_configuration(pgm1_loc).run().value();

    // test of asm_options
    EXPECT_EQ("SEVEN", analyzer_opts.opts.sysparm);
    EXPECT_EQ("MAC1", analyzer_opts.opts.profile);

    const auto& pp_options = analyzer_opts.pp_opts;
    EXPECT_TRUE(pp_options.size() == 1 && std::holds_alternative<db2_preprocessor_options>(pp_options.front()));

    // test of asm_options override
    auto [analyzer_opts_override, __] = ws_cfg.get_analyzer_configuration(pgm_override_loc).run().value();
    EXPECT_EQ("SEVEN", analyzer_opts_override.opts.sysparm);
    EXPECT_EQ("PROFILE OVERRIDE", analyzer_opts_override.opts.profile);

    // test sysin options in workspace
    auto asm_options_ws = ws_cfg.get_analyzer_configuration(pgm_anything_loc).run().value().first.opts;
    EXPECT_EQ(asm_options_ws.sysin_dsn, "pgms");
    EXPECT_EQ(asm_options_ws.sysin_member, "anything");

    // test sysin options out of workspace
    auto asm_options_ows = ws_cfg.get_analyzer_configuration(pgm_outside_ws).run().value().first.opts;
    EXPECT_EQ(asm_options_ows.sysin_dsn, is_windows() ? "c:\\Users\\outside" : "/home/user/outside");
    EXPECT_EQ(asm_options_ows.sysin_member, "anything");
}

class file_manager_asm_test : public file_manager_proc_grps_test
{
public:
    hlasm_plugin::utils::value_task<std::optional<std::string>> get_file_content(
        const resource_location& location) override
    {
        if (hlasm_plugin::utils::resource::filename(location) == "proc_grps.json")
            return hlasm_plugin::utils::value_task<std::optional<std::string>>::from_value(R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [],
      "asm_options": {
         "GOFF":true,
         "XOBJECT":true
      }
    }
  ]
})");
        else
            return file_manager_proc_grps_test::get_file_content(location);
    }
};

TEST(workspace_configuration, asm_options_goff_xobject_redefinition)
{
    file_manager_asm_test file_manager;
    shared_json global_settings = make_empty_shared_json();
    lib_config global_config;
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, global_config, nullptr, nullptr);

    ws_cfg.parse_configuration_file().run();

    std::vector<diagnostic> diags;
    ws_cfg.produce_diagnostics(diags, {}, {});

    EXPECT_TRUE(contains_message_codes(diags, { "W0002" }));
}

TEST(workspace_configuration, watcher_registrations)
{
    NiceMock<file_manager_mock> file_manager;
    EXPECT_CALL(file_manager, get_file_content(_)).WillRepeatedly(Invoke([]() {
        return hlasm_plugin::utils::value_task<std::optional<std::string>>::from_value(std::nullopt);
    }));
    EXPECT_CALL(file_manager, list_directory_subdirs_and_symlinks(_))
        .WillOnce(Return(hlasm_plugin::utils::value_task<
            hlasm_plugin::parser_library::workspaces::list_directory_result>::from_value({
            {},
            hlasm_plugin::utils::path::list_directory_rc::not_a_directory,
        })));

    shared_json global_settings(std::make_shared<nlohmann::json>(R"({
    "hlasm": {
    "pgm_conf": {"pgms":[]},
    "proc_grps": {
      "pgroups": [
        {
          "name": "test",
          "libs": [
            "scheme:/test/library",
            "scheme:/test/pattern/*/"
          ]
        }
      ]
    }}})"_json));
    lib_config global_config;

    struct watcher_registration_provider_mock : watcher_registration_provider
    {
        MOCK_METHOD(watcher_registration_id, add_watcher, (std::string_view uri, bool recursive), (override));
        MOCK_METHOD(void, remove_watcher, (watcher_registration_id id), (override));
    } wrp;

    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, global_config, nullptr, &wrp);

    EXPECT_CALL(wrp, add_watcher("scheme:/test/library/", false)).WillOnce(Return(watcher_registration_id(1)));
    EXPECT_CALL(wrp, add_watcher("scheme:/test/pattern/", true)).WillOnce(Return(watcher_registration_id(2)));

    ws_cfg.parse_configuration_file().run();

    global_settings.store(std::make_shared<nlohmann::json>(R"({
    "hlasm": {
    "pgm_conf": {"pgms":[]},
    "proc_grps": { "pgroups": [ ] }}})"_json));

    EXPECT_CALL(wrp, remove_watcher(watcher_registration_id(1)));
    EXPECT_CALL(wrp, remove_watcher(watcher_registration_id(2)));

    EXPECT_TRUE(ws_cfg.settings_updated());

    ws_cfg.parse_configuration_file().run();
}

namespace {
class file_manager_refresh_needed_test : public file_manager_impl
{
public:
    hlasm_plugin::utils::value_task<std::optional<std::string>> get_file_content(
        const resource_location& location) override
    {
        using hlasm_plugin::utils::value_task;
        if (location.get_uri().ends_with("proc_grps.json"))
            return value_task<std::optional<std::string>>::from_value(m_file_proc_grps_content);
        else if (location.get_uri().ends_with("pgm_conf.json"))
            return value_task<std::optional<std::string>>::from_value(m_file_pgm_conf_content);
        else
            return value_task<std::optional<std::string>>::from_value(std::nullopt);
    }

private:
    const std::string m_file_proc_grps_content = R"({
    "pgroups": [
        {
            "name": "P1",
            "libs": [
                "test://workspace/externals/library0",
                "test://workspace/externals/library1",
                "test://workspace/externals/library1/SYSTEM_LIBRARY",
                "test://workspace/externals/library2",
                "test://workspace/externals/library3"
            ]
        }
    ]
})";

    const std::string m_file_pgm_conf_content = R"({
  "pgms": [
    {
      "program": "pgm1",
      "pgroup": "P1"
    }
  ]

})";
};

class refresh_needed_test : public Test
{
public:
    void SetUp() override
    {
        EXPECT_EQ(m_cfg.parse_configuration_file().run().value(), parse_config_file_result::parsed);

        cache_content(resource_location("test://workspace/pgm1"),
            {
                resource_location("test://workspace/externals/library1/"),
                resource_location("test://workspace/externals/library1/SYSTEM_LIBRARY/"),
                resource_location("test://workspace/externals/library2/"),
            });
    }

    bool refresh_libs(const std::vector<resource_location>& uris)
    {
        auto refreshed_pgroups = m_cfg.refresh_libraries(uris).run().value();
        return refreshed_pgroups.has_value() && refreshed_pgroups->size() == 1;
    }

private:
    file_manager_refresh_needed_test m_fm;
    const resource_location ws_loc = resource_location("test://workspace/");
    const shared_json m_global_settings = make_empty_shared_json();
    lib_config m_global_config;
    workspace_configuration m_cfg =
        workspace_configuration(m_fm, ws_loc, m_global_settings, m_global_config, nullptr, nullptr);

    void cache_content(
        const resource_location& pgm_location, const std::unordered_set<resource_location>& lib_locations_to_cache)
    {
        for (const auto& lib : m_cfg.get_proc_grp_by_program(*m_cfg.get_program(pgm_location))->libraries())
        {
            if (auto it = lib_locations_to_cache.find(lib->get_location()); it != lib_locations_to_cache.end())
                lib->prefetch().run();
        }
    }
};
} // namespace

TEST_F(refresh_needed_test, not_refreshed)
{
    // different scheme
    EXPECT_FALSE(refresh_libs({ resource_location("aaa://workspace/externals/library1") }));

    // different path
    EXPECT_FALSE(refresh_libs({ resource_location("test://workspace/something/else") }));
    EXPECT_FALSE(refresh_libs({ resource_location("test://workspace/externals/library1/MAC/") }));
    EXPECT_FALSE(refresh_libs({ resource_location("test://workspace/externals/library1/mac/") }));

    // not used
    EXPECT_FALSE(refresh_libs({ resource_location("test://workspace/externals/library") }));
    EXPECT_FALSE(refresh_libs({ resource_location("test://workspace/externals/library/") }));
    EXPECT_FALSE(refresh_libs({ resource_location("test://workspace/externals/library4") }));
    EXPECT_FALSE(refresh_libs({ resource_location("test://workspace/externals/library4/") }));
    EXPECT_FALSE(refresh_libs({ resource_location("test://home") }));
    EXPECT_FALSE(refresh_libs({ resource_location("test://home/") }));

    // not cached
    EXPECT_FALSE(refresh_libs({ resource_location("test://workspace/externals/library3") }));
    EXPECT_FALSE(refresh_libs({ resource_location("test://workspace/externals/library3/") }));
}

TEST_F(refresh_needed_test, refreshed)
{
    // New element
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace/externals/library1/MAC") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace/externals/library1/mac") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace/externals/library1/SYSTEM_LIBRARY/MAC") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace/externals/library1/SYSTEM_LIBRARY/mac") }));

    // whole tree gets deleted
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace/externals/library2") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace/externals/library2/") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace/externals") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace/externals/") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace/") }));

    EXPECT_TRUE(refresh_libs({ resource_location("test://home"), resource_location("test://workspace") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://home/"), resource_location("test://workspace/") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace"), resource_location("test://home") }));
    EXPECT_TRUE(refresh_libs({ resource_location("test://workspace/"), resource_location("test://home/") }));

    EXPECT_TRUE(refresh_libs({
        resource_location("test://workspace/externals/library1"),
        resource_location("test://workspace/externals/library2"),
        resource_location("test://workspace/externals/library3"),
    }));
}
