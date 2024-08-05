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

#include "diagnostic.h"

#include "utils/concat.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library {

using hlasm_plugin::utils::concat;

namespace {
std::string get_not_defined_proc_group_msg(std::string_view config_file, std::string_view pgroup)
{
    return concat("The ",
        config_file,
        " file refers to a processor group \"",
        pgroup,
        "\", that is not defined in proc_grps.json");
}
} // namespace

diagnostic error_W0001(const utils::resource::resource_location& file_name)
{
    return diagnostic(std::string(file_name.get_uri()),
        {},
        diagnostic_severity::error,
        "W0001",
        concat("Could not read file ", file_name.to_presentable()),
        {},
        diagnostic_tag::none);
}

diagnostic error_W0002(const utils::resource::resource_location& ws_uri)
{
    return diagnostic(std::string(ws_uri.get_uri()),
        {},
        diagnostic_severity::error,
        "W0002",
        concat("Malformed proc_conf configuration file."),
        {},
        diagnostic_tag::none);
}

diagnostic error_W0003(const utils::resource::resource_location& file_name)
{
    return diagnostic(std::string(file_name.get_uri()),
        {},
        diagnostic_severity::error,
        "W0003",
        concat("Malformed pgm_conf configuration file."),
        {},
        diagnostic_tag::none);
}

diagnostic error_W0004(const utils::resource::resource_location& file_name, std::string_view pgroup)
{
    return diagnostic(std::string(file_name.get_uri()),
        {},
        diagnostic_severity::error,
        "W0004",
        get_not_defined_proc_group_msg("pgm_conf.json", pgroup),
        {},
        diagnostic_tag::none);
}

diagnostic error_W0005(
    const utils::resource::resource_location& file_name, std::string_view name, std::string_view type)
{
    return diagnostic(std::string(file_name.get_uri()),
        {},
        diagnostic_severity::warning,
        "W0005",
        concat(
            "The ", type, " '", name, "' from '", file_name.to_presentable(), "' defines invalid assembler options."),
        {},
        diagnostic_tag::none);
}

diagnostic error_W0006(
    const utils::resource::resource_location& file_name, std::string_view proc_group, std::string_view type)
{
    return diagnostic(std::string(file_name.get_uri()),
        {},
        diagnostic_severity::warning,
        "W0006",
        concat("The processor group '",
            proc_group,
            "' from '",
            file_name.to_presentable(),
            "' defines invalid ",
            type,
            " preprocessor options."),
        {},
        diagnostic_tag::none);
}

diagnostic warn_W0007(const utils::resource::resource_location& file_name, std::string_view substitution)
{
    return diagnostic(std::string(file_name.get_uri()),
        {},
        diagnostic_severity::warning,
        "W0007",
        concat("Unable to perform workspace settings substitution for variable '", substitution, "'."),
        {},
        diagnostic_tag::none);
}

diagnostic warn_W0008(const utils::resource::resource_location& file_name, std::string_view pgroup)
{
    return diagnostic(std::string(file_name.get_uri()),
        {},
        diagnostic_severity::warning,
        "W0008",
        get_not_defined_proc_group_msg("pgm_conf.json", pgroup),
        {},
        diagnostic_tag::none);
}

diagnostic error_B4G001(const utils::resource::resource_location& file_name)
{
    return diagnostic(std::string(file_name.get_uri()),
        {},
        diagnostic_severity::warning,
        "B4G001",
        concat("The .bridge.json file has unexpected content"),
        {},
        diagnostic_tag::none);
}

diagnostic error_B4G002(const utils::resource::resource_location& file_name, std::string_view grp_name)
{
    return diagnostic(std::string(file_name.get_uri()),
        {},
        diagnostic_severity::error,
        "B4G002",
        get_not_defined_proc_group_msg(".bridge.json", grp_name),
        {},
        diagnostic_tag::none);
}

diagnostic warn_B4G003(const utils::resource::resource_location& file_name, std::string_view grp_name)
{
    return diagnostic(std::string(file_name.get_uri()),
        {},
        diagnostic_severity::warning,
        "B4G003",
        get_not_defined_proc_group_msg(".bridge.json", grp_name),
        {},
        diagnostic_tag::none);
}

diagnostic info_SUP(std::string file_name)
{
    return diagnostic(std::move(file_name),
        range(position(), position(0, position::max_value)),
        diagnostic_severity::hint,
        "SUP",
        concat("Diagnostics suppressed, no configuration available."),
        {},
        diagnostic_tag::none);
}

diagnostic error_L0001(
    const utils::resource::resource_location& config_loc, const utils::resource::resource_location& lib_loc)
{
    return diagnostic(std::string(config_loc.get_uri()),
        {},
        "L0001",
        concat("Unable to load library: ", lib_loc.to_presentable(), "."));
}

diagnostic error_L0002(
    const utils::resource::resource_location& config_loc, const utils::resource::resource_location& lib_loc)
{
    return diagnostic(std::string(config_loc.get_uri()),
        {},
        "L0002",
        concat("Unable to load library: ", lib_loc.to_presentable(), ". Error: The path does not point to directory."));
}

diagnostic warning_L0004(const utils::resource::resource_location& config_loc,
    const utils::resource::resource_location& lib_loc,
    std::string_view macro_name,
    bool has_extensions)
{
    return diagnostic(std::string(config_loc.get_uri()),
        {},
        diagnostic_severity::warning,
        "L0004",
        concat("Library '",
            lib_loc.to_presentable(),
            "' contains conflicting macro definitions (",
            macro_name,
            "). Consider ",
            has_extensions ? std::string_view("changing") : std::string_view("adding"),
            " 'macro_extensions' parameter."),
        {},
        diagnostic_tag::none);
}

diagnostic warning_L0005(const utils::resource::resource_location& config_loc, std::string_view pattern, size_t limit)
{
    return diagnostic(std::string(config_loc.get_uri()),
        {},
        diagnostic_severity::warning,
        "L0005",
        concat("Limit of ", limit, " directories was reached while evaluating library pattern '", pattern, "'."),
        {},
        diagnostic_tag::none);
}

diagnostic warning_L0006(const utils::resource::resource_location& config_loc, std::string_view path)
{
    return diagnostic(std::string(config_loc.get_uri()),
        {},
        diagnostic_severity::warning,
        "L0006",
        concat("Home directory could not have been retrieved while expanding '", path, "'."),
        {},
        diagnostic_tag::none);
}

} // namespace hlasm_plugin::parser_library
