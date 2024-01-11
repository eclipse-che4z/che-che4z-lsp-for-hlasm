/*
 * Copyright (c) 2021 Broadcom.
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

#include "proc_grps.h"

#include <array>

#include "assembler_options.h"
#include "instruction_set_version.h"
#include "nlohmann/json.hpp"

namespace hlasm_plugin::parser_library::config {

bool preprocessor_options::valid() const noexcept
{
    return std::visit([](const auto& t) { return t.valid(); }, options);
}

std::string_view preprocessor_options::type() const noexcept
{
    return std::visit([]<typename T>(const T&) { return T::name; }, options);
}

void to_json(nlohmann::json& j, const processor_group_root_folder& p)
{
    j = p == processor_group_root_folder::alternate_root;
}
void from_json(const nlohmann::json& j, processor_group_root_folder& p)
{
    bool v = false;
    j.get_to(v);
    p = v ? processor_group_root_folder::alternate_root : processor_group_root_folder::workspace;
}

void to_json(nlohmann::json& j, const library& p)
{
    j = nlohmann::json { { "path", p.path }, { "optional", p.optional } };
    if (auto m = nlohmann::json(p.macro_extensions); !m.empty())
        j["macro_extensions"] = std::move(m);
    if (p.root_folder != processor_group_root_folder {})
        j["prefer_alternate_root"] = p.root_folder;
}
void from_json(const nlohmann::json& j, library& p)
{
    if (j.is_string())
        j.get_to(p.path);
    else if (j.is_object())
    {
        j.at("path").get_to(p.path);
        if (auto it = j.find("optional"); it != j.end())
            it->get_to(p.optional);
        else
            p.optional = false;

        if (auto it = j.find("macro_extensions"); it != j.end())
            it->get_to(p.macro_extensions);
        else
            p.macro_extensions.clear();

        if (auto it = j.find("prefer_alternate_root"); it != j.end())
            it->get_to(p.root_folder);
        else
            p.root_folder = processor_group_root_folder::workspace;
    }
    else
        throw nlohmann::json::other_error::create(501, "Unexpected JSON type.", &j);
}

void to_json(nlohmann::json& j, const dataset& p)
{
    j = nlohmann::json { { "dataset", p.dsn }, { "optional", p.optional } };
}

void from_json(const nlohmann::json& j, dataset& p)
{
    if (!j.is_object())
        throw nlohmann::json::other_error::create(501, "Unexpected JSON type.", &j);

    j.at("dataset").get_to(p.dsn);
    if (auto it = j.find("optional"); it != j.end())
        it->get_to(p.optional);
    else
        p.optional = false;
}

void to_json(nlohmann::json& j, const endevor_dataset& p)
{
    j = nlohmann::json {
        { "dataset", p.dsn },
        { "optional", p.optional },
        { "profile", p.profile },
    };
}

void from_json(const nlohmann::json& j, endevor_dataset& p)
{
    if (!j.is_object())
        throw nlohmann::json::other_error::create(501, "Unexpected JSON type.", &j);

    j.at("dataset").get_to(p.dsn);
    if (auto it = j.find("optional"); it != j.end())
        p.optional = it->get_to(p.optional);
    else
        p.optional = false;

    if (auto it = j.find("profile"); it != j.end())
        it->get_to(p.profile);
    else
        p.profile.clear();
}

void to_json(nlohmann::json& j, const endevor& p)
{
    j = nlohmann::json {
        { "environment", p.environment },
        { "stage", p.stage },
        { "system", p.system },
        { "subsystem", p.subsystem },
        { "type", p.type },
        { "use_map", p.use_map },
        { "optional", p.optional },
        { "profile", p.profile },
    };
}
void from_json(const nlohmann::json& j, endevor& p)
{
    if (!j.is_object())
        throw nlohmann::json::other_error::create(501, "Unexpected JSON type.", &j);

    j.at("environment").get_to(p.environment);
    j.at("stage").get_to(p.stage);
    j.at("system").get_to(p.system);
    j.at("subsystem").get_to(p.subsystem);
    j.at("type").get_to(p.type);

    if (auto it = j.find("use_map"); it != j.end())
        it->get_to(p.use_map);
    else
        p.use_map = true;

    if (auto it = j.find("optional"); it != j.end())
        it->get_to(p.optional);
    else
        p.optional = false;

    if (auto it = j.find("profile"); it != j.end())
        it->get_to(p.profile);
    else
        p.profile.clear();
}

template<typename T>
bool to_json_preprocessor_defaults(nlohmann::json& j, const T& v)
{
    static const T default_config;
    if (v == default_config)
    {
        j = T::name;
        return true;
    }

    j = nlohmann::json {
        { "name", T::name },
    };
    return false;
}

void to_json(nlohmann::json& j, const db2_preprocessor& v)
{
    if (to_json_preprocessor_defaults(j, v))
        return;

    j["options"] = nlohmann::json {
        { "conditional", v.conditional },
        { "version", v.version },
    };
}
void from_json(const nlohmann::json& j, db2_preprocessor& v)
{
    v = db2_preprocessor {};
    if (!j.is_object())
        return;
    if (auto it = j.find("options"); it != j.end())
    {
        if (!it->is_object())
            throw nlohmann::json::other_error::create(501, "Object with DB2 options expected.", &j);
        if (auto ver = it->find("version"); ver != it->end())
        {
            if (!ver->is_string())
                throw nlohmann::json::other_error::create(501, "Version string expected.", &j);
            v.version = ver->get<std::string>();
        }
        if (auto cond = it->find("conditional"); cond != it->end())
        {
            if (!cond->is_boolean())
                throw nlohmann::json::other_error::create(501, "Boolean expected.", &j);
            v.conditional = cond->get<bool>();
        }
    }
}

void to_json(nlohmann::json& j, const cics_preprocessor& v)
{
    if (to_json_preprocessor_defaults(j, v))
        return;

    j["options"] = nlohmann::json::array({
        v.prolog ? "PROLOG" : "NOPROLOG",
        v.epilog ? "EPILOG" : "NOEPILOG",
        v.leasm ? "LEASM" : "NOLEASM",
    });
}

namespace {
const std::map<std::string_view, std::pair<bool(cics_preprocessor::*), bool>, std::less<>> cics_preprocessor_options = {
    { "PROLOG", { &cics_preprocessor::prolog, true } },
    { "NOPROLOG", { &cics_preprocessor::prolog, false } },
    { "EPILOG", { &cics_preprocessor::epilog, true } },
    { "NOEPILOG", { &cics_preprocessor::epilog, false } },
    { "LEASM", { &cics_preprocessor::leasm, true } },
    { "NOLEASM", { &cics_preprocessor::leasm, false } },
};
}

void from_json(const nlohmann::json& j, cics_preprocessor& v)
{
    v = cics_preprocessor {};
    if (!j.is_object())
        return;
    if (auto it = j.find("options"); it != j.end())
    {
        if (!it->is_array())
            throw nlohmann::json::other_error::create(501, "Array of CICS options expected.", &j);
        for (const auto& e : *it)
        {
            if (!e.is_string())
                throw nlohmann::json::other_error::create(501, "CICS option expected.", &j);
            if (auto cpo = cics_preprocessor_options.find(e.get<std::string_view>());
                cpo != cics_preprocessor_options.end())
            {
                const auto [member, value] = cpo->second;
                v.*member = value;
            }
        }
    }
}

void to_json(nlohmann::json& j, const endevor_preprocessor& v)
{
    if (to_json_preprocessor_defaults(j, v))
        return;
}
void from_json(const nlohmann::json&, endevor_preprocessor& v) { v = endevor_preprocessor(); }

namespace {
struct preprocessor_visitor
{
    nlohmann::json& j;

    template<typename T>
    void operator()(const T& p) const
    {
        j = p;
    }
};
} // namespace

void to_json(nlohmann::json& j, const std::variant<library, dataset, endevor, endevor_dataset>& v)
{
    std::visit([&j](const auto& x) { j = x; }, v);
}
void from_json(const nlohmann::json& j, std::variant<library, dataset, endevor, endevor_dataset>& v)
{
    if (j.contains("dataset"))
    {
        if (!j.contains("profile"))
            v = j.get<dataset>();
        else
            v = j.get<endevor_dataset>();
    }
    else if (j.contains("subsystem"))
        v = j.get<endevor>();
    else
        v = j.get<library>();
}

void to_json(nlohmann::json& j, const processor_group& p)
{
    j = nlohmann::json { { "name", p.name }, { "libs", p.libs } };
    if (auto opts = nlohmann::json(p.asm_options); !opts.empty())
        j["asm_options"] = std::move(opts);

    if (p.preprocessors.empty())
    {
        // nothing to do
    }
    else if (p.preprocessors.size() == 1)
    {
        std::visit(preprocessor_visitor { j["preprocessor"] }, p.preprocessors.front().options);
    }
    else
    {
        auto& pp_array = j["preprocessor"] = nlohmann::json::array_t();
        for (const auto& pp : p.preprocessors)
            std::visit(preprocessor_visitor { pp_array.emplace_back() }, pp.options);
    }
}

namespace {
template<typename T>
T& emplace_options(std::vector<preprocessor_options>& preprocessors)
{
    return std::get<T>(preprocessors.emplace_back(preprocessor_options { T() }).options);
}

template<typename T>
constexpr auto generate_preprocessor_entry()
{
    return std::make_pair(
        T::name, +[](std::vector<preprocessor_options>& preprocessors, const nlohmann::json& j) {
            j.get_to(emplace_options<T>(preprocessors));
        });
}

template<typename T>
struct preprocessor_list_generator;
template<typename... Ts>
struct preprocessor_list_generator<std::variant<Ts...>>
{
    static constexpr const std::array list = { generate_preprocessor_entry<Ts>()... };
};

constexpr const auto preprocessor_list = preprocessor_list_generator<decltype(preprocessor_options::options)>::list;

constexpr auto find_preprocessor_deserializer(std::string_view name)
{
    auto it = std::find_if(std::begin(preprocessor_list), std::end(preprocessor_list), [&name](const auto& entry) {
        return entry.first == name;
    });
    return it != std::end(preprocessor_list) ? it->second : nullptr;
}

void add_single_preprocessor(std::vector<preprocessor_options>& preprocessors, const nlohmann::json& j)
{
    std::string p_name;
    if (j.is_string())
        p_name = j.get<std::string>();
    else if (j.is_object())
        j.at("name").get_to(p_name);

    std::transform(p_name.begin(), p_name.end(), p_name.begin(), [](unsigned char c) { return (char)toupper(c); });
    if (auto deserializer = find_preprocessor_deserializer(p_name); deserializer)
        deserializer(preprocessors, j);
    else
        throw nlohmann::json::other_error::create(501, "Unable to identify requested preprocessor.", &j);
}

} // namespace

void from_json(const nlohmann::json& j, processor_group& p)
{
    j.at("name").get_to(p.name);
    j.at("libs").get_to(p.libs);
    if (auto it = j.find("asm_options"); it != j.end())
        it->get_to(p.asm_options);

    if (auto it = j.find("preprocessor"); it != j.end())
    {
        if (it->is_array())
        {
            for (const auto& nested_j : *it)
                add_single_preprocessor(p.preprocessors, nested_j);
        }
        else
            add_single_preprocessor(p.preprocessors, *it);
    }
}

void to_json(nlohmann::json& j, const proc_grps& p)
{
    j = nlohmann::json { { "pgroups", p.pgroups } };
    if (auto m = nlohmann::json(p.macro_extensions); !m.empty())
        j["macro_extensions"] = std::move(m);
}
void from_json(const nlohmann::json& j, proc_grps& p)
{
    j.at("pgroups").get_to(p.pgroups);
    if (auto it = j.find("macro_extensions"); it != j.end())
        it->get_to(p.macro_extensions);
}

} // namespace hlasm_plugin::parser_library::config
