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

#include "workspace_configuration.h"

#include <charconv>

#include "file_manager.h"
#include "library_local.h"
#include "nlohmann/json.hpp"
#include "utils/content_loader.h"
#include "utils/encoding.h"
#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"
#include "wildcard.h"

namespace hlasm_plugin::parser_library::workspaces {
namespace {
std::optional<std::filesystem::path> get_fs_abs_path(std::string_view path)
{
    if (path.empty())
        return std::nullopt;

    try
    {
        if (std::filesystem::path fs_path = path; utils::path::is_absolute(fs_path))
            return fs_path;

        return std::nullopt;
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

utils::resource::resource_location transform_to_resource_location(
    std::string_view path, const utils::resource::resource_location& base_resource_location)
{
    using utils::resource::resource_location;
    resource_location rl;

    if (resource_location::is_local(path))
        rl = resource_location("file:" + utils::encoding::percent_encode_and_ignore_utf8(path.substr(5)));
    else if (utils::path::is_uri(path))
        rl = resource_location(path);
    else if (auto fs_path = get_fs_abs_path(path); fs_path.has_value())
        rl = resource_location(utils::path::path_to_uri(utils::path::lexically_normal(*fs_path).string()));
    else if (base_resource_location.is_local())
        rl = resource_location::join(base_resource_location, utils::encoding::percent_encode(path));
    else
        rl = resource_location::join(base_resource_location, path);

    return rl.lexically_normal();
}

std::vector<std::string> get_macro_extensions_compatibility_list(std::span<const std::string> always_recognize)
{
    // Extract extension list for compatibility reasons
    std::vector<std::string> macro_extensions_compatibility_list;
    for (const auto& wildcard : always_recognize)
    {
        std::string_view wc(wildcard);
        if (const auto ext_pattern = wc.rfind("*."); ext_pattern != std::string_view::npos)
        {
            wc.remove_prefix(ext_pattern + 1);
            macro_extensions_compatibility_list.emplace_back(wc);
        }
    }

    std::sort(macro_extensions_compatibility_list.begin(), macro_extensions_compatibility_list.end());

    macro_extensions_compatibility_list.erase(
        std::unique(macro_extensions_compatibility_list.begin(), macro_extensions_compatibility_list.end()),
        macro_extensions_compatibility_list.end());

    return macro_extensions_compatibility_list;
}

std::optional<std::string> substitute_home_directory(std::string p)
{
    if (!p.starts_with("~"))
        return p;

    const auto& homedir = utils::platform::home();
    if (homedir.empty())
        return std::nullopt;

    const auto skip = (size_t)1 + (p.starts_with("~/") || p.starts_with("~\\"));
    return utils::path::join(homedir, std::move(p).substr(skip)).string();
}

library_local_options get_library_local_options(const config::library& lib,
    std::span<const std::string> fallback_macro_extensions,
    std::span<const std::string> always_recognize)
{
    library_local_options opts;

    opts.optional_library = lib.optional;
    if (!lib.macro_extensions.empty())
        opts.extensions = lib.macro_extensions;
    else if (!fallback_macro_extensions.empty())
        opts.extensions = std::vector(fallback_macro_extensions.begin(), fallback_macro_extensions.end());
    else if (auto macro_extensions_compatibility_list = get_macro_extensions_compatibility_list(always_recognize);
             !macro_extensions_compatibility_list.empty())
    {
        opts.extensions = macro_extensions_compatibility_list;
        opts.extensions_from_deprecated_source = true;
    }

    return opts;
}

const nlohmann::json* find_member(std::string_view key, const nlohmann::json& j)
{
    if (j.is_object())
    {
        if (auto it = j.find(key); it == j.end())
            return nullptr;
        else
            return &it.value();
    }
    else if (j.is_array())
    {
        unsigned long long i = 0;
        const auto conv_result = std::from_chars(std::to_address(key.begin()), std::to_address(key.end()), i);
        if (conv_result.ec != std::errc() || conv_result.ptr != std::to_address(key.end()) || i >= j.size())
            return nullptr;

        return &j[i];
    }
    else
        return nullptr;
}

std::optional<std::string_view> find_setting(std::string_view key, const nlohmann::json& m_j)
{
    const nlohmann::json* j = &m_j;

    while (true)
    {
        auto dot = key.find('.');
        auto subkey = key.substr(0, dot);

        j = find_member(subkey, *j);
        if (!j)
            return std::nullopt;

        if (dot == std::string_view::npos)
            break;
        else
            key.remove_prefix(dot + 1);
    }

    if (!j->is_string())
        return std::nullopt;
    else
        return j->get<std::string_view>();
}

struct json_settings_replacer
{
    static const std::regex config_reference;

    const nlohmann::json& global_settings;
    global_settings_map& utilized_settings_values;
    const utils::resource::resource_location& location;

    std::match_results<std::string_view::iterator> matches;
    std::unordered_set<std::string, utils::hashers::string_hasher, std::equal_to<>> unavailable;

    void operator()(nlohmann::json& val)
    {
        if (val.is_structured())
        {
            for (auto& value : val)
                (*this)(value);
        }
        else if (val.is_string())
        {
            const auto& value = val.get<std::string_view>();

            if (auto replacement = try_replace(value); replacement.has_value())
                val = std::move(replacement).value();
        }
    }

    std::optional<std::string> try_replace(std::string_view s)
    {
        std::optional<std::string> result;
        if (!std::regex_search(s.begin(), s.end(), matches, config_reference))
            return result;

        auto& r = result.emplace();
        do
        {
            r.append(s.begin(), matches[0].first);
            s.remove_prefix(matches[0].second - s.begin());

            static constexpr std::string_view config_section = "config:";

            std::string_view key(std::to_address(matches[1].first), matches[1].length());
            if (key.starts_with(config_section))
            {
                auto reduced_key = key.substr(config_section.size());
                auto v = find_setting(reduced_key, global_settings);
                if (v.has_value())
                    r.append(*v);
                else
                    unavailable.emplace(key);
                utilized_settings_values.emplace(reduced_key, v);
            }
            else if (key == "workspaceFolder")
                r.append(location.get_path()); // TODO: change to get_uri as soon as possible
            else
                unavailable.emplace(key);

        } while (std::regex_search(s.begin(), s.end(), matches, config_reference));

        r.append(s);

        return result;
    }
};

const std::regex json_settings_replacer::config_reference(R"(\$\{([^}]+)\})");

} // namespace

workspace_configuration::workspace_configuration(
    file_manager& fm, utils::resource::resource_location location, const shared_json& global_settings)
    : m_file_manager(fm)
    , m_location(std::move(location))
    , m_global_settings(global_settings)
{
    auto hlasm_folder = utils::resource::resource_location::join(m_location, HLASM_PLUGIN_FOLDER);
    m_proc_grps_loc = utils::resource::resource_location::join(hlasm_folder, FILENAME_PROC_GRPS);
    m_pgm_conf_loc = utils::resource::resource_location::join(hlasm_folder, FILENAME_PGM_CONF);
}

bool workspace_configuration::is_configuration_file(const utils::resource::resource_location& file) const
{
    return is_config_file(file) || is_b4g_config_file(file);
}
void workspace_configuration::process_processor_group(const config::processor_group& pg,
    std::span<const std::string> fallback_macro_extensions,
    std::span<const std::string> always_recognize,
    const utils::resource::resource_location& alternative_root,
    std::vector<diagnostic_s>& diags)
{
    processor_group prc_grp(pg.name, pg.asm_options, pg.preprocessors);

    for (auto& lib : pg.libs)
    {
        const auto& root =
            lib.root_folder == config::processor_group_root_folder::alternate_root && !alternative_root.empty()
            ? alternative_root
            : m_location;

        std::optional<std::string> lib_path = substitute_home_directory(lib.path);
        if (!lib_path.has_value())
        {
            diags.push_back(diagnostic_s::warning_L0006(m_proc_grps_loc, lib.path));
            continue;
        }

        auto lib_local_opts = get_library_local_options(lib, fallback_macro_extensions, always_recognize);
        auto rl = transform_to_resource_location(*lib_path, root);
        rl.join(""); // Ensure that this is a directory

        if (auto first_wild_card = rl.get_uri().find_first_of("*?"); first_wild_card == std::string::npos)
            prc_grp.add_library(std::make_unique<library_local>(
                m_file_manager, std::move(rl), std::move(lib_local_opts), m_proc_grps_loc));
        else
            find_and_add_libs(utils::resource::resource_location(
                                  rl.get_uri().substr(0, rl.get_uri().find_last_of("/", first_wild_card) + 1)),
                rl,
                prc_grp,
                lib_local_opts,
                diags);
    }
    m_proc_grps.try_emplace(std::make_pair(prc_grp.name(), alternative_root), std::move(prc_grp));
}

bool workspace_configuration::process_program(const config::program_mapping& pgm, std::vector<diagnostic_s>& diags)
{
    const auto key = std::make_pair(pgm.pgroup, utils::resource::resource_location());
    if (!m_proc_grps.contains(key))
        return false;

    std::optional<std::string> pgm_name = substitute_home_directory(pgm.program);
    if (!pgm_name.has_value())
    {
        diags.push_back(diagnostic_s::warning_L0006(m_pgm_conf_loc, pgm.program));
        return false;
    }

    if (auto rl = transform_to_resource_location(*pgm_name, m_location);
        pgm_name->find_first_of("*?") == std::string::npos)
        m_exact_pgm_conf.try_emplace(rl, tagged_program { program(rl, key, pgm.opts) });
    else
        m_regex_pgm_conf.emplace_back(tagged_program { program { rl, key, pgm.opts } }, wildcard2regex(rl.get_uri()));

    return true;
}

bool workspace_configuration::is_config_file(const utils::resource::resource_location& file) const
{
    return file == m_proc_grps_loc || file == m_pgm_conf_loc;
}

bool workspace_configuration::is_b4g_config_file(const utils::resource::resource_location& file) const
{
    return file.get_uri().ends_with(B4G_CONF_FILE) && utils::resource::filename(file) == B4G_CONF_FILE;
}

// open config files and parse them
parse_config_file_result workspace_configuration::load_and_process_config(std::vector<diagnostic_s>& diags)
{
    diags.clear();

    config::proc_grps proc_groups;
    file_ptr proc_grps_file;
    global_settings_map utilized_settings_values;

    if (auto l = load_proc_config(proc_groups, proc_grps_file, utilized_settings_values, diags);
        l != parse_config_file_result::parsed)
        return l;

    m_proc_grps.clear();
    m_exact_pgm_conf.clear();
    m_regex_pgm_conf.clear();
    m_b4g_config_cache.clear();

    config::pgm_conf pgm_config;
    file_ptr pgm_conf_file;
    const auto pgm_conf_loaded = load_pgm_config(pgm_config, pgm_conf_file, utilized_settings_values, diags);

    // process processor groups
    for (const auto& pg : proc_groups.pgroups)
    {
        process_processor_group(
            pg, proc_groups.macro_extensions, pgm_config.always_recognize, utils::resource::resource_location(), diags);
    }

    if (pgm_conf_loaded != parse_config_file_result::parsed)
    {
        m_local_config = {};
    }
    else
    {
        m_local_config = lib_config::load_from_pgm_config(pgm_config);

        // process programs
        for (const auto& pgm : pgm_config.pgms)
        {
            if (!process_program(pgm, diags))
                diags.push_back(diagnostic_s::error_W0004(pgm_conf_file->get_location(), pgm.pgroup));
        }
    }

    m_utilized_settings_values = std::move(utilized_settings_values);
    m_proc_grps_source = std::move(proc_groups);

    // we need to tolerate pgm_conf processing failure, because other products may provide the info
    return parse_config_file_result::parsed;
}

parse_config_file_result workspace_configuration::load_proc_config(config::proc_grps& proc_groups,
    file_ptr& proc_grps_file,
    global_settings_map& utilized_settings_values,
    std::vector<diagnostic_s>& diags)
{
    const auto current_settings = m_global_settings.load();
    json_settings_replacer json_visitor { *current_settings, utilized_settings_values, m_location };

    // proc_grps.json parse
    proc_grps_file = m_file_manager.add_file(m_proc_grps_loc);
    if (proc_grps_file->update_and_get_bad() == update_file_result::bad)
        return parse_config_file_result::not_found;

    try
    {
        auto proc_json = nlohmann::json::parse(proc_grps_file->get_text());
        json_visitor(proc_json);
        proc_json.get_to(proc_groups);
    }
    catch (const nlohmann::json::exception&)
    {
        // could not load proc_grps
        diags.push_back(diagnostic_s::error_W0002(proc_grps_file->get_location()));
        return parse_config_file_result::error;
    }

    for (const auto& var : json_visitor.unavailable)
        diags.push_back(diagnostic_s::warn_W0007(proc_grps_file->get_location(), var));

    for (const auto& pg : proc_groups.pgroups)
    {
        if (!pg.asm_options.valid())
            diags.push_back(diagnostic_s::error_W0005(proc_grps_file->get_location(), pg.name, "processor group"));
        for (const auto& p : pg.preprocessors)
        {
            if (!p.valid())
                diags.push_back(diagnostic_s::error_W0006(proc_grps_file->get_location(), pg.name, p.type()));
        }
    }

    return parse_config_file_result::parsed;
}

parse_config_file_result workspace_configuration::load_pgm_config(config::pgm_conf& pgm_config,
    file_ptr& pgm_conf_file,
    global_settings_map& utilized_settings_values,
    std::vector<diagnostic_s>& diags)
{
    const auto current_settings = m_global_settings.load();
    json_settings_replacer json_visitor { *current_settings, utilized_settings_values, m_location };

    // pgm_conf.json parse
    pgm_conf_file = m_file_manager.add_file(m_pgm_conf_loc);
    if (pgm_conf_file->update_and_get_bad() == update_file_result::bad)
        return parse_config_file_result::not_found;

    try
    {
        auto pgm_json = nlohmann::json::parse(pgm_conf_file->get_text());
        json_visitor(pgm_json);
        pgm_json.get_to(pgm_config);
    }
    catch (const nlohmann::json::exception&)
    {
        diags.push_back(diagnostic_s::error_W0003(pgm_conf_file->get_location()));
        return parse_config_file_result::error;
    }

    for (const auto& var : json_visitor.unavailable)
        diags.push_back(diagnostic_s::warn_W0007(pgm_conf_file->get_location(), var));

    for (const auto& pgm : pgm_config.pgms)
    {
        if (!pgm.opts.valid())
            diags.push_back(diagnostic_s::error_W0005(pgm_conf_file->get_location(), pgm.program, "program"));
    }

    return parse_config_file_result::parsed;
}

bool workspace_configuration::settings_updated() const
{
    auto global_settings = m_global_settings.load();
    for (const auto& [key, value] : m_utilized_settings_values)
    {
        if (find_setting(key, *global_settings) != value)
            return true;
    }
    return false;
}

parse_config_file_result workspace_configuration::parse_b4g_config_file(
    const utils::resource::resource_location& file_location)
{
    // keep in sync with try_loading_alternative_configuration
    const auto alternative_root =
        utils::resource::resource_location::replace_filename(file_location, "").join("..").lexically_normal();

    auto [it, inserted] = m_b4g_config_cache.try_emplace(file_location);
    if (!inserted)
    {
        std::erase_if(m_exact_pgm_conf, [tag = &*it](const auto& e) { return e.second.tag == tag; });
        std::erase_if(m_regex_pgm_conf, [tag = &*it](const auto& e) { return e.first.tag == tag; });
        std::erase_if(m_proc_grps, [&alternative_root](const auto& e) { return e.first.second == alternative_root; });
        it->second = {};
    }

    auto b4g_config_file = m_file_manager.add_file(file_location);
    if (b4g_config_file->update_and_get_bad() == update_file_result::bad)
        return parse_config_file_result::not_found;

    const void* new_tag = &*it;
    auto& conf = it->second;
    std::unordered_set<std::string> missing_pgroups;
    try
    {
        conf.config.emplace(nlohmann::json::parse(b4g_config_file->get_text()).get<config::b4g_map>());
    }
    catch (const nlohmann::json::exception&)
    {
        conf.diags.push_back(diagnostic_s::error_B4G001(file_location));
        return parse_config_file_result::error;
    }

    for (const auto& pg_def : m_proc_grps_source.pgroups)
        process_processor_group(pg_def, m_proc_grps_source.macro_extensions, {}, alternative_root, conf.diags);

    for (const auto& [name, details] : conf.config.value().files)
    {
        proc_grp_id grp_id(details.processor_group_name, alternative_root);
        if (!m_proc_grps.contains(grp_id))
        {
            missing_pgroups.emplace(details.processor_group_name);
            continue;
        }
        const auto filename = utils::resource::resource_location::replace_filename(file_location, name);
        m_exact_pgm_conf.try_emplace(filename,
            tagged_program {
                program {
                    filename,
                    std::move(grp_id),
                    {},
                },
                new_tag,
            });
    }
    if (const auto& def_grp = conf.config.value().default_processor_group_name; !def_grp.empty())
    {
        proc_grp_id grp_id(def_grp, alternative_root);
        if (!m_proc_grps.contains(grp_id))
            missing_pgroups.emplace(def_grp);
        else
        {
            auto rl = utils::resource::resource_location::replace_filename(file_location, "*");
            m_regex_pgm_conf.emplace_back(
                tagged_program {
                    program { rl, std::move(grp_id), {} },
                    new_tag,
                },
                wildcard2regex(rl.get_uri()));
        }
    }

    for (const auto& pgroup : missing_pgroups)
        conf.diags.push_back(diagnostic_s::error_B4G002(file_location, pgroup));

    return parse_config_file_result::parsed;
}


void workspace_configuration::find_and_add_libs(const utils::resource::resource_location& root,
    const utils::resource::resource_location& path_pattern,
    processor_group& prc_grp,
    const library_local_options& opts,
    std::vector<diagnostic_s>& diags)
{
    if (!m_file_manager.dir_exists(root))
    {
        if (!opts.optional_library)
            diags.push_back(diagnostic_s::error_L0001(m_proc_grps_loc, root));
        return;
    }

    std::regex path_validator = percent_encoded_pathmask_to_regex(path_pattern.get_uri());

    std::unordered_set<std::string> processed_canonical_paths;
    std::deque<std::pair<std::string, utils::resource::resource_location>> dirs_to_search;

    if (std::error_code ec; dirs_to_search.emplace_back(m_file_manager.canonical(root, ec), root), ec)
    {
        if (!opts.optional_library)
            diags.push_back(diagnostic_s::error_L0001(m_proc_grps_loc, root));
        return;
    }

    constexpr size_t limit = 1000;
    while (!dirs_to_search.empty())
    {
        if (processed_canonical_paths.size() > limit)
        {
            diags.push_back(diagnostic_s::warning_L0005(m_proc_grps_loc, path_pattern.to_presentable(), limit));
            break;
        }

        auto [canonical_path, dir] = std::move(dirs_to_search.front());
        dirs_to_search.pop_front();

        if (!processed_canonical_paths.insert(std::move(canonical_path)).second)
            continue;

        if (std::regex_match(dir.get_uri(), path_validator))
            prc_grp.add_library(std::make_unique<library_local>(m_file_manager, dir, opts, m_proc_grps_loc));

        auto [subdir_list, return_code] = m_file_manager.list_directory_subdirs_and_symlinks(dir);
        if (return_code != utils::path::list_directory_rc::done)
        {
            diags.push_back(diagnostic_s::error_L0001(m_proc_grps_loc, dir));
            break;
        }

        for (auto& [subdir_canonical_path, subdir] : subdir_list)
        {
            if (processed_canonical_paths.contains(subdir_canonical_path))
                continue;

            dirs_to_search.emplace_back(std::move(subdir_canonical_path), subdir.lexically_normal());
        }
    }
}

void workspace_configuration::copy_diagnostics(const diagnosable& target,
    const std::unordered_set<utils::resource::resource_location, utils::resource::resource_location_hasher>& b4g_filter)
    const
{
    for (auto& [_, pg] : m_proc_grps)
    {
        pg.collect_diags();
        for (const auto& d : pg.diags())
            target.add_diagnostic(d);
        pg.diags().clear();
    }

    for (const auto& diag : m_config_diags)
        target.add_diagnostic(diag);

    for (const auto& [uri, c] : m_b4g_config_cache)
    {
        if (!b4g_filter.contains(uri))
            continue;
        for (const auto& d : c.diags)
            target.add_diagnostic(d);
    }
}

parse_config_file_result workspace_configuration::parse_configuration_file(
    std::optional<utils::resource::resource_location> file)
{
    if (!file.has_value() || is_config_file(*file))
        return load_and_process_config(m_config_diags);

    if (is_b4g_config_file(*file))
        return parse_b4g_config_file(*file);


    return parse_config_file_result::not_found;
}

bool workspace_configuration::refresh_libraries(const std::vector<utils::resource::resource_location>& file_locations)
{
    bool refreshed = false;
    for (auto& [_, proc_grp] : m_proc_grps)
    {
        if (!proc_grp.refresh_needed(file_locations))
            continue;
        refreshed = true;
        for (auto& lib : proc_grp.libraries())
        {
            lib->refresh();
        }
        proc_grp.generate_suggestions();
    }
    return refreshed;
}

const processor_group& workspace_configuration::get_proc_grp_by_program(const program& pgm) const
{
    return m_proc_grps.at(pgm.pgroup);
}

processor_group& workspace_configuration::get_proc_grp_by_program(const program& pgm)
{
    return m_proc_grps.at(pgm.pgroup);
}

const processor_group& workspace_configuration::get_proc_grp(const proc_grp_id& p) const { return m_proc_grps.at(p); }

const program* workspace_configuration::get_program(const utils::resource::resource_location& file_location) const
{
    return get_program_normalized(file_location.lexically_normal());
}

const program* workspace_configuration::get_program_normalized(
    const utils::resource::resource_location& file_location_normalized) const
{
    // direct match
    if (auto program = m_exact_pgm_conf.find(file_location_normalized); program != m_exact_pgm_conf.cend())
        return &program->second.pgm;

    for (const auto& [program, pattern] : m_regex_pgm_conf)
    {
        if (std::regex_match(file_location_normalized.get_uri(), pattern))
            return &program.pgm;
    }
    return nullptr;
}

std::pair<parse_config_file_result, utils::resource::resource_location>
workspace_configuration::try_loading_alternative_configuration(const utils::resource::resource_location& file_location)
{
    auto result = std::pair(parse_config_file_result::parsed,
        utils::resource::resource_location::replace_filename(file_location, B4G_CONF_FILE));
    auto& [parsed, configuration_url] = result;

    if (auto it = m_b4g_config_cache.find(configuration_url); it == m_b4g_config_cache.end())
        parsed = parse_b4g_config_file(configuration_url);
    else if (!it->second.config.has_value()) // keep in sync with parse_b4g_config_file
        parsed = it->second.diags.empty() ? parse_config_file_result::not_found : parse_config_file_result::error;

    return result;
}

utils::resource::resource_location workspace_configuration::load_alternative_config_if_needed(
    const utils::resource::resource_location& file_location)
{
    const auto rl = file_location.lexically_normal();

    if (get_program_normalized(rl))
        return utils::resource::resource_location();

    auto [result, uri] = try_loading_alternative_configuration(rl);

    if (result == parse_config_file_result::not_found)
        return utils::resource::resource_location();

    return std::move(uri);
}

} // namespace hlasm_plugin::parser_library::workspaces
