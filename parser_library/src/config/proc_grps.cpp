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

#include "nlohmann/json.hpp"

namespace hlasm_plugin::parser_library::config {

void to_json(nlohmann::json& j, const library& p)
{
    j = nlohmann::json { { "path", p.path }, { "optional", p.optional } };
    if (auto m = nlohmann::json(p.macro_extensions); !m.empty())
        j["macro_extensions"] = std::move(m);
}
void from_json(const nlohmann::json& j, library& p)
{
    if (j.is_string())
        j.get_to(p.path);
    else if (j.is_object())
    {
        j.at("path").get_to(p.path);
        if (auto it = j.find("optional"); it != j.end())
            p.optional = it->get_to(p.optional);
        if (auto it = j.find("macro_extensions"); it != j.end())
            it->get_to(p.macro_extensions);
    }
    else
        throw nlohmann::json::other_error::create(501, "Unexpected JSON type.");
}

void to_json(nlohmann::json& j, const assembler_options& p)
{
    j = nlohmann::json::object();
    if (p.profile.size())
        j["PROFILE"] = p.profile;
    if (p.sysparm.size())
        j["SYSPARM"] = p.sysparm;
    if (p.system_id.size())
        j["SYSTEM_ID"] = p.system_id;
}
void from_json(const nlohmann::json& j, assembler_options& p)
{
    if (!j.is_object())
        throw nlohmann::json::other_error::create(501, "asm_options must be an object.");

    if (auto it = j.find("PROFILE"); it != j.end())
        it->get_to(p.profile);
    if (auto it = j.find("SYSPARM"); it != j.end())
        it->get_to(p.sysparm);
    if (auto it = j.find("SYSTEM_ID"); it != j.end())
        it->get_to(p.system_id);
}

void to_json(nlohmann::json& j, const db2_preprocessor& v)
{
    static const db2_preprocessor default_config;
    if (v == default_config)
    {
        j = "DB2";
        return;
    }
    j = nlohmann::json {
        { "name", "DB2" },
        {
            "options",
            {
                { "version", v.version },
            },
        },
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
            throw nlohmann::json::other_error::create(501, "Object with DB2 options expected.");
        if (auto ver = it->find("version"); ver != it->end())
        {
            if (!ver->is_string())
                throw nlohmann::json::other_error::create(501, "Version string expected.");
            v.version = ver->get<std::string>();
        }
    }
}

void to_json(nlohmann::json& j, const cics_preprocessor& v)
{
    static const cics_preprocessor default_config;
    if (v == default_config)
    {
        j = "CICS";
        return;
    }

    j = nlohmann::json {
        { "name", "CICS" },
        {
            "options",
            nlohmann::json::array({
                v.prolog ? "PROLOG" : "NOPROLOG",
                v.epilog ? "EPILOG" : "NOEPILOG",
                v.leasm ? "LEASM" : "NOLEASM",
            }),
        },
    };
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
            throw nlohmann::json::other_error::create(501, "Array of CICS options expected.");
        for (const auto& e : *it)
        {
            if (!e.is_string())
                throw nlohmann::json::other_error::create(501, "CICS option expected.");
            if (auto cpo = cics_preprocessor_options.find(e.get<std::string_view>());
                cpo != cics_preprocessor_options.end())
            {
                const auto [member, value] = cpo->second;
                v.*member = value;
            }
        }
    }
}

namespace {
struct preprocessor_visitor
{
    nlohmann::json& j;

    void operator()(const std::monostate&) const {}
    void operator()(const db2_preprocessor& p) const { j = p; }
    void operator()(const cics_preprocessor& p) const { j = p; }
};
} // namespace

void to_json(nlohmann::json& j, const processor_group& p)
{
    j = nlohmann::json { { "name", p.name }, { "libs", p.libs } };
    if (auto opts = nlohmann::json(p.asm_options); !opts.empty())
        j["asm_options"] = std::move(opts);

    if (!std::holds_alternative<std::monostate>(p.preprocessor.options))
        std::visit(preprocessor_visitor { j["preprocessor"] }, p.preprocessor.options);
}
void from_json(const nlohmann::json& j, processor_group& p)
{
    j.at("name").get_to(p.name);
    j.at("libs").get_to(p.libs);
    if (auto it = j.find("asm_options"); it != j.end())
        it->get_to(p.asm_options);

    if (auto it = j.find("preprocessor"); it != j.end())
    {
        std::string p_name;
        if (it->is_string())
            p_name = it->get<std::string>();
        else if (it->is_object())
            it->at("name").get_to(p_name);
        else
            throw nlohmann::json::other_error::create(501, "Unable to identify requested preprocessor.");

        std::transform(p_name.begin(), p_name.end(), p_name.begin(), [](unsigned char c) { return (char)toupper(c); });
        if (p_name == "DB2")
            it->get_to(p.preprocessor.options.emplace<db2_preprocessor>());
        else if (p_name == "CICS")
            it->get_to(p.preprocessor.options.emplace<cics_preprocessor>());
        else
            throw nlohmann::json::other_error::create(501, "Unable to identify requested preprocessor.");
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

namespace {
struct preprocessor_validator
{
    template<typename T>
    bool operator()(const T& t) const noexcept
    {
        return t.valid();
    }

    bool operator()(const std::monostate&) const noexcept { return true; }
};
} // namespace

bool preprocessor_options::valid() const noexcept { return std::visit(preprocessor_validator {}, options); }

} // namespace hlasm_plugin::parser_library::config
