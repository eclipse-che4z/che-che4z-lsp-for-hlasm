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

#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_IMPL_H

#include "diagnosable_impl.h"
#include "file.h"
#include "processor.h"

namespace hlasm_plugin::parser_library::workspaces {

#pragma warning(push)
#pragma warning(disable : 4250)

// Implementation of the file interface. Implements LSP incremental changing
// of contents of file as well as loading the file from disk.
class file_impl : public virtual file, public virtual diagnosable_impl
{
public:
    explicit file_impl(file_uri uri);
    explicit file_impl(const file_impl&) = default;
    file_impl& operator=(const file_impl&) = default;

    file_impl(file_impl&&) = default;
    file_impl& operator=(file_impl&&) = default;

    virtual void collect_diags() const override;

    virtual const file_uri& get_file_name() override;
    virtual const std::string& get_text() override;
    virtual version_t get_version() override;
    virtual bool update_and_get_bad() override;
    virtual bool get_lsp_editing() override;

    virtual void did_open(std::string new_text, version_t version) override;
    virtual void did_change(std::string new_text) override;
    virtual void did_change(range range, std::string new_text) override;
    virtual void did_close() override;

    static std::string replace_non_utf8_chars(const std::string& text);

    virtual ~file_impl() = default;

protected:
    const std::string& get_text_ref();

private:
    file_uri file_name_;
    std::string text_;
    // Array of "pointers" to text_ where lines start.
    std::vector<size_t> lines_ind_;

    bool up_to_date_ = false;
    bool editing_ = false;
    bool bad_ = false;

    version_t version_ = 0;

    void load_text();

    size_t index_from_location(position pos) const;
};

#pragma warning(pop)

} // namespace hlasm_plugin::parser_library::workspaces

#endif // !HLASMPLUGIN_PARSERLIBRARY_FILE_IMPL_H
