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
#ifndef TEST_WORKSPACE_WORKSPACE_FILE_WITH_TEXT_TEST_H
#define TEST_WORKSPACE_WORKSPACE_FILE_WITH_TEXT_TEST_H

#include "workspaces/processor_file_impl.h"

class file_with_text : public hlasm_plugin::parser_library::workspaces::processor_file_impl
{
public:
    file_with_text(const std::string& name,
        const std::string& text,
        const hlasm_plugin::parser_library::workspaces::file_manager& file_mngr)
        : file_impl(name)
        , processor_file_impl(name, file_mngr)
    {
        did_open(text, 1);
    }

    virtual const std::string& get_text() override { return get_text_ref(); }

    virtual bool update_and_get_bad() override { return false; }
};

#endif
