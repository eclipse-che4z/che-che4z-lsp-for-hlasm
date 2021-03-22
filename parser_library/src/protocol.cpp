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

#include "protocol.h"

#include "debugging/debug_types.h"
#include "diagnosable.h"
#include "semantics/highlighting_info.h"
#include "semantics/lsp_info_processor.h"
#include "workspaces/processor.h"

namespace hlasm_plugin::parser_library {
string_array::string_array(const char** arr, size_t size)
    : arr(arr)
    , size(size) {};

num_array::num_array(size_t* arr, size_t size)
    : arr(arr)
    , size(size) {};

completion_item::completion_item(context::completion_item_s& info)
    : impl_(info)
{}

const char* completion_item::label() const { return impl_.label.c_str(); }
size_t completion_item::kind() const { return impl_.kind; }
const char* completion_item::detail() const { return impl_.detail.c_str(); }
const char* completion_item::documentation()
{
    impl_.implode_contents();
    return impl_.content_string.c_str();
}
bool completion_item::deprecated() const { return impl_.deprecated; }
const char* completion_item::insert_text() const { return impl_.insert_text.c_str(); }

completion_list::completion_list(semantics::completion_list_s& info)
    : impl_(info)
{}

bool completion_list::is_incomplete() const { return impl_.is_incomplete; }
completion_item completion_list::item(size_t index) { return impl_.items[index]; }
size_t completion_list::count() const { return impl_.items.size(); }

position_uri::position_uri(semantics::position_uri_s& info)
    : impl_(info)
{}

position position_uri::pos() const { return impl_.pos; }

const char* position_uri::uri() const { return impl_.uri.c_str(); }

diagnostic_related_info::diagnostic_related_info(diagnostic_related_info_s& info)
    : impl_(info)
{}

range_uri::range_uri(range_uri_s& range)
    : impl_(range)
{}

range range_uri::get_range() const { return impl_.rang; }

const char* range_uri::uri() const { return impl_.uri.c_str(); }


range_uri diagnostic_related_info::location() const { return impl_.location; }

const char* diagnostic_related_info::message() const { return impl_.message.c_str(); }

diagnostic::diagnostic(diagnostic_s& diag)
    : impl_(diag)
{}

const char* diagnostic::file_name() const { return impl_.file_name.c_str(); }

range diagnostic::get_range() const { return impl_.diag_range; }

diagnostic_severity diagnostic::severity() const { return impl_.severity; }

const char* diagnostic::code() const { return impl_.code.c_str(); }

const char* diagnostic::source() const { return impl_.source.c_str(); }

const char* diagnostic::message() const { return impl_.message.c_str(); }

const diagnostic_related_info diagnostic::related_info(size_t index) const { return impl_.related[index]; }

size_t diagnostic::related_info_size() const { return impl_.related.size(); }

//********************* diagnostics_container *******************

class diagnostic_list_impl
{
public:
    std::vector<diagnostic_s> diags;
};

diagnostic_list::diagnostic_list()
    : begin_(nullptr)
    , size_(0)
{}

diagnostic_list::diagnostic_list(diagnostic_s* begin, size_t size)
    : begin_(begin)
    , size_(size)
{}

diagnostic diagnostic_list::diagnostics(size_t index) { return begin_[index]; }

size_t diagnostic_list::diagnostics_size() const { return size_; }

position_uris::position_uris(semantics::position_uri_s* data, size_t size)
    : data_(data)
    , size_(size)
{}

position_uri position_uris::get_position_uri(size_t index) { return data_[index]; }
size_t position_uris::size() const { return size_; }

token_info::token_info(const range& token_range, semantics::hl_scopes scope)
    : token_range(token_range)
    , scope(scope) {};
token_info::token_info(
    size_t line_start, size_t column_start, size_t line_end, size_t column_end, semantics::hl_scopes scope)
    : token_range({ { line_start, column_start }, { line_end, column_end } })
    , scope(scope) {};
//*********************** stack_frame *************************
stack_frame::stack_frame(const debugging::stack_frame& frame)
    : name(frame.name)
    , source_file(frame.frame_source)
    , source_range { { frame.begin_line, 0 }, { frame.end_line, 0 } }
    , id(frame.id)
{}

template<>
stack_frame sequence<stack_frame, const debugging::stack_frame*>::item(size_t index) const
{
    return stack_frame(stor_[index]);
}

//********************* source **********************

source::source(const debugging::source& source)
    : path(source.path)
{}

//*********************** scope *************************

scope::scope(const debugging::scope& impl)
    : name(impl.name)
    , variable_reference(impl.var_reference)
    , source_file(impl.scope_source)
{}

template<>
scope sequence<scope, const debugging::scope*>::item(size_t index) const
{
    return scope(stor_[index]);
}


//********************** variable **********************

variable::variable(const debugging::variable& impl)
    : name(impl.get_name())
    , value(impl.get_value())
    , variable_reference(impl.var_reference)
    , type(impl.type())
{}

template<>
variable sequence<variable, const debugging::variable_store*>::item(size_t index) const
{
    return variable(*stor_->variables[index]);
}



} // namespace hlasm_plugin::parser_library
