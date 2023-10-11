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
#include "fade_messages.h"
#include "location.h"
#include "lsp/completion_item.h"
#include "lsp/document_symbol_item.h"
#include "semantics/highlighting_info.h"

namespace hlasm_plugin::parser_library {

//********************** completion item **********************

completion_item::completion_item(const lsp::completion_item_s& item)
    : item_(item)
{}

std::string_view completion_item::label() const { return item_.label; }
completion_item_kind completion_item::kind() const { return item_.kind; }
std::string_view completion_item::detail() const { return item_.detail; }
std::string_view completion_item::documentation() const { return item_.documentation; }
std::string_view completion_item::insert_text() const { return item_.insert_text; }
bool completion_item::is_snippet() const { return item_.snippet; }
std::string_view completion_item::suggestion_for() const { return item_.suggestion_for; }

completion_item sequence_item_get(const sequence<completion_item, const lsp::completion_item_s*>* self, size_t index)
{
    return completion_item(self->stor_[index]);
}

//********************** document symbol item **********************

document_symbol_item::document_symbol_item(const lsp::document_symbol_item_s& item)
    : item_(item)
{}

sequence<char> document_symbol_item::name() const { return sequence<char>(item_.name); }
document_symbol_kind document_symbol_item::kind() const { return item_.kind; }
range document_symbol_item::symbol_range() const { return item_.symbol_range; }
range document_symbol_item::symbol_selection_range() const { return item_.symbol_selection_range; }
document_symbol_list document_symbol_item::children() const
{
    return document_symbol_list(item_.children.data(), item_.children.size());
}

document_symbol_item sequence_item_get(
    const sequence<document_symbol_item, const lsp::document_symbol_item_s*>* self, size_t index)
{
    return document_symbol_item(self->stor_[index]);
}
//********************** location **********************

position_uri::position_uri(const location& item)
    : item_(item)
{}
position position_uri::pos() const { return item_.pos; }
std::string_view position_uri::file_uri() const { return item_.get_uri(); }

position_uri sequence_item_get(const sequence<position_uri, const location*>* self, size_t index)
{
    return position_uri(self->stor_[index]);
}

diagnostic_related_info::diagnostic_related_info(diagnostic_related_info_s& info)
    : impl_(info)
{}

range_uri::range_uri(range_uri_s& range)
    : impl_(range)
{}

range range_uri::get_range() const { return impl_.rang; }

const char* range_uri::uri() const { return impl_.uri.c_str(); }

//********************** diagnostic **********************

range_uri diagnostic_related_info::location() const { return range_uri(impl_.location); }

const char* diagnostic_related_info::message() const { return impl_.message.c_str(); }

diagnostic::diagnostic(diagnostic_s& diag)
    : impl_(diag)
{}

const char* diagnostic::file_uri() const { return impl_.file_uri.c_str(); }

range diagnostic::get_range() const { return impl_.diag_range; }

diagnostic_severity diagnostic::severity() const { return impl_.severity; }

const char* diagnostic::code() const { return impl_.code.c_str(); }

const char* diagnostic::source() const { return impl_.source.c_str(); }

const char* diagnostic::message() const { return impl_.message.c_str(); }

const diagnostic_related_info diagnostic::related_info(size_t index) const { return impl_.related[index]; }

size_t diagnostic::related_info_size() const { return impl_.related.size(); }

diagnostic_tag diagnostic::tags() const { return impl_.tag; }

//********************** fade message **********************

fade_message::fade_message(fade_message_s& fm)
    : impl_(fm)
{}

const char* fade_message::file_uri() const { return impl_.uri.c_str(); }

range fade_message::get_range() const { return impl_.r; }

const char* fade_message::code() const { return impl_.code.c_str(); }

const char* fade_message::source() const { return fade_message_s::source.data(); }

const char* fade_message::message() const { return impl_.message.c_str(); }

//********************* diagnostics_container *******************

diagnostic_list::diagnostic_list()
    : begin_(nullptr)
    , size_(0)
{}

diagnostic_list::diagnostic_list(diagnostic_s* begin, size_t size)
    : begin_(begin)
    , size_(size)
{}

diagnostic diagnostic_list::diagnostics(size_t index) { return static_cast<diagnostic>(begin_[index]); }

size_t diagnostic_list::diagnostics_size() const { return size_; }

//********************* fade message list *******************

fade_message_list::fade_message_list()
    : begin_(nullptr)
    , size_(0)
{}

fade_message_list::fade_message_list(fade_message_s* begin, size_t size)
    : begin_(begin)
    , size_(size)
{}

fade_message fade_message_list::message(size_t index) { return static_cast<fade_message>(begin_[index]); }

size_t fade_message_list::size() const { return size_; }

//*********************** stack_frame *************************
stack_frame::stack_frame(const debugging::stack_frame& frame)
    : name(frame.name)
    , source_file(frame.frame_source)
    , source_range { { frame.begin_line, 0 }, { frame.end_line, 0 } }
    , id(frame.id)
{}

stack_frame sequence_item_get(const sequence<stack_frame, const debugging::stack_frame*>* self, size_t index)
{
    return stack_frame(self->stor_[index]);
}

//********************* source **********************

source::source(const debugging::source& source)
    : uri(source.uri)
{}

//*********************** scope *************************

scope::scope(const debugging::scope& impl)
    : name(impl.name)
    , variable_reference(impl.var_reference)
    , source_file(impl.scope_source)
{}

scope sequence_item_get(const sequence<scope, const debugging::scope*>* self, size_t index)
{
    return scope(self->stor_[index]);
}


//********************** variable **********************

variable::variable(const debugging::variable& impl)
    : name(impl.get_name())
    , value(impl.get_value())
    , variable_reference(impl.var_reference)
    , type(impl.type())
{}

variable sequence_item_get(const sequence<variable, const debugging::variable_store*>* self, size_t index)
{
    return variable(*self->stor_->variables[index]);
}



} // namespace hlasm_plugin::parser_library
