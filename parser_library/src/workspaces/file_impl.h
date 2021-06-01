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

    void collect_diags() const override;

    const file_uri& get_file_name() override;
    const std::string& get_text() override;
    version_t get_version() override;
    bool update_and_get_bad() override;
    bool get_lsp_editing() override;

    void did_open(std::string new_text, version_t version) override;
    void did_change(std::string new_text) override;
    void did_change(range range, std::string new_text) override;
    void did_close() override;

    static std::string replace_non_utf8_chars(const std::string& text);
    static std::vector<size_t> create_line_indices(const std::string& text);

    // Returns the location in text that corresponds to utf-16 based location
    // The position may point beyond the last character -> returns text.size()
    static size_t index_from_position(const std::string& text, const std::vector<size_t>& line_indices, position pos);

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
};

#pragma warning(pop)

} // namespace hlasm_plugin::parser_library::workspaces

#endif // !HLASMPLUGIN_PARSERLIBRARY_FILE_IMPL_H
