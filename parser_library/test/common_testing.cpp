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

#include "common_testing.h"

#include "hlasmparser_multiline.h"
#include "utils/resource_location.h"
#include "utils/task.h"
#include "workspace_manager.h"
#include "workspaces/workspace.h"

void parse_all_files(hlasm_plugin::parser_library::workspaces::workspace& ws)
{
    for (auto t = ws.parse_file(); t.valid(); t = ws.parse_file())
        t.run();
}

void run_if_valid(hlasm_plugin::utils::task t)
{
    if (t.valid())
        t.run();
}

void open_parse_and_recollect_diags(
    workspace& ws, const std::vector<hlasm_plugin::utils::resource::resource_location>& files)
{
    std::for_each(files.begin(), files.end(), [&ws](const hlasm_plugin::utils::resource::resource_location& f) {
        run_if_valid(ws.did_open_file(f));
    });
    parse_all_files(ws);

    ws.diags().clear();
    ws.collect_diags();
}

void close_parse_and_recollect_diags(
    workspace& ws, const std::vector<hlasm_plugin::utils::resource::resource_location>& files)
{
    std::for_each(files.begin(), files.end(), [&ws](const hlasm_plugin::utils::resource::resource_location& f) {
        run_if_valid(ws.did_close_file(f));
    });
    parse_all_files(ws);

    ws.diags().clear();
    ws.collect_diags();
}
