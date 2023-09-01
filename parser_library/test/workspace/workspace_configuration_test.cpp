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
#include "utils/resource_location.h"
#include "utils/task.h"
#include "workspaces/file_manager_impl.h"
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

    EXPECT_CALL(fm, get_file_content(_))
        .WillRepeatedly(Invoke([](const auto&) -> hlasm_plugin::utils::value_task<std::optional<std::string>> {
            co_return std::nullopt;
        }));

    workspace_configuration cfg(fm, resource_location("test://workspace"), global_settings, nullptr);

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

    workspace_configuration cfg(fm, resource_location("test://workspace"), global_settings, &ext_confg);
    cfg.parse_configuration_file().run();

    EXPECT_CALL(ext_confg,
        read_external_configuration(
            Truly([](sequence<char> v) { return std::string_view(v) == "test://workspace/file1.hlasm"; }), _))
        .WillOnce(Invoke([](auto, auto channel) { channel.provide(sequence<char>(std::string_view(R"("GRP1")"))); }));

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
    NiceMock<external_configuration_requests_mock> ext_confg;

    EXPECT_CALL(fm, get_file_content(_)).WillRepeatedly(Invoke([]() {
        return value_task<std::optional<std::string>>::from_value(std::nullopt);
    }));

    workspace_configuration cfg(fm, resource_location("test://workspace"), global_settings, &ext_confg);
    cfg.parse_configuration_file().run();

    EXPECT_CALL(ext_confg,
        read_external_configuration(
            Truly([](sequence<char> v) { return std::string_view(v) == "test://workspace/file1.hlasm"; }), _))
        .WillOnce(Invoke([](auto, auto channel) {
            channel.provide(sequence<char>(std::string_view(R"({
      "name": "GRP1",
      "libs": [
        "path"
      ],
      "asm_options": {"SYSPARM": "PARM1"}
    })")));
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
    NiceMock<external_configuration_requests_mock> ext_confg;

    EXPECT_CALL(fm, get_file_content(_)).WillRepeatedly(Invoke([]() {
        return value_task<std::optional<std::string>>::from_value(std::nullopt);
    }));

    workspace_configuration cfg(fm, resource_location("test://workspace"), global_settings, &ext_confg);
    cfg.parse_configuration_file().run();

    static constexpr std::string_view grp_def(R"({
      "name": "GRP1",
      "libs": [
        "path"
      ],
      "asm_options": {"SYSPARM": "PARM1"}
    })");

    EXPECT_CALL(ext_confg,
        read_external_configuration(
            Truly([](sequence<char> v) { return std::string_view(v) == "test://workspace/file1.hlasm"; }), _))
        .WillOnce(Invoke([](auto, auto channel) { channel.provide(sequence<char>(grp_def)); }));

    const resource_location pgm_loc("test://workspace/file1.hlasm");

    cfg.load_alternative_config_if_needed(pgm_loc).run();

    EXPECT_CALL(ext_confg,
        read_external_configuration(
            Truly([](sequence<char> v) { return std::string_view(v) == "test://workspace/file2.hlasm"; }), _))
        .WillOnce(Invoke([](auto, auto channel) { channel.provide(sequence<char>(grp_def)); }));

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
    NiceMock<external_configuration_requests_mock> ext_confg;

    EXPECT_CALL(fm, get_file_content(_)).WillRepeatedly(Invoke([]() {
        return value_task<std::optional<std::string>>::from_value(std::nullopt);
    }));

    workspace_configuration cfg(fm, resource_location("test://workspace"), global_settings, &ext_confg);
    cfg.parse_configuration_file().run();

    static constexpr std::string_view grp_def(R"({
      "name": "GRP1",
      "libs": [
        "path"
      ],
      "asm_options": {"SYSPARM": "PARM1"}
    })");

    EXPECT_CALL(ext_confg,
        read_external_configuration(
            Truly([](sequence<char> v) { return std::string_view(v) == "test://workspace/file1.hlasm"; }), _))
        .WillOnce(Invoke([](auto, auto channel) { channel.provide(sequence<char>(grp_def)); }));

    const resource_location pgm_loc("test://workspace/file1.hlasm");

    cfg.load_alternative_config_if_needed(pgm_loc).run();

    cfg.prune_external_processor_groups(resource_location());

    const auto* pgm = cfg.get_program(pgm_loc);

    EXPECT_EQ(pgm, nullptr);

    EXPECT_THROW(cfg.get_proc_grp(external_conf { std::make_shared<std::string>(grp_def) }), std::out_of_range);
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
    workspace_configuration m_cfg = workspace_configuration(m_fm, ws_loc, m_global_settings, nullptr);

    void cache_content(const resource_location& pgm_location,
        const std::unordered_set<resource_location, resource_location_hasher>& lib_locations_to_cache)
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
