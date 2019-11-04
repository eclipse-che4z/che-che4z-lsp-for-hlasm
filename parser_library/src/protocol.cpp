#include "shared/protocol.h"
#include "diagnosable.h"
#include "semantics/highlighting_info.h"
#include "semantics/lsp_info_processor.h"
#include "processor.h"
#include "debugging/debug_types.h"

namespace hlasm_plugin::parser_library
{
string_array::string_array(const char** arr, size_t size) : arr(arr), size(size) {};

completion_item::completion_item(context::completion_item_s& info) : impl_(info) {}

const char* completion_item::label()
{
	return impl_.label.c_str();
}
size_t completion_item::kind()
{
	return impl_.kind;
}
const char* completion_item::detail()
{
	return impl_.detail.c_str();
}
const char* completion_item::documentation()
{
	return impl_.documentation.c_str();
}
bool completion_item::deprecated()
{
	return impl_.deprecated;
}
const char* completion_item::insert_text()
{
	return impl_.insert_text.c_str();
}

completion_list::completion_list(semantics::completion_list_s& info) : impl_(info) {}

bool completion_list::is_incomplete()
{
	return impl_.is_incomplete;
}
completion_item completion_list::item(size_t index)
{
	return impl_.items[index];
}
size_t completion_list::count()
{
	return impl_.items.size();
}

position_uri::position_uri(semantics::position_uri_s & info) : impl_(info)
{}

position position_uri::pos()
{
	return impl_.pos;
}

const char * position_uri::uri()
{
	return impl_.uri.c_str();
}

diagnostic_related_info::diagnostic_related_info(diagnostic_related_info_s & info) : impl_(info)
{

}

range_uri::range_uri(range_uri_s & range):impl_(range) {}

range range_uri::get_range()
{
	return impl_.rang;
}

const char * range_uri::uri()
{
	return impl_.uri.c_str();
}


range_uri diagnostic_related_info::location() const
{
	return impl_.location;
}

const char * diagnostic_related_info::message() const
{
	return impl_.message.c_str();
}

diagnostic::diagnostic(diagnostic_s & diag) : impl_(diag)
{
}

const char * diagnostic::file_name()
{
	return impl_.file_name.c_str();
}

range diagnostic::get_range()
{
	return impl_.diag_range;
}

diagnostic_severity diagnostic::severity()
{
	return impl_.severity;
}

const char * diagnostic::code()
{
	return impl_.code.c_str();
}

const char * diagnostic::source()
{
	return impl_.source.c_str();
}

const char * diagnostic::message()
{
	return impl_.message.c_str();
}

const diagnostic_related_info diagnostic::related_info(size_t index) const
{
	return impl_.related[index];
}

size_t diagnostic::related_info_size()
{
	return impl_.related.size();
}

//*********************** file_higlighting_info *****************
file_highlighting_info::file_highlighting_info(semantics::highlighting_info & info) :info(info) {}

const char * file_highlighting_info::document_uri()
{
	return info.document.uri.c_str();
}

version_t file_highlighting_info::document_version()
{
	return info.document.version;
}

token_info file_highlighting_info::token(size_t index)
{
	return info.lines[index];
}

size_t file_highlighting_info::token_count()
{
	return info.lines.size();
}

position file_highlighting_info::continuation(size_t index)
{
	return info.cont_info.continuation_positions[index];
}

size_t file_highlighting_info::continuation_count()
{
	return info.cont_info.continuation_positions.size();
}

size_t file_highlighting_info::continuation_column()
{
	return info.cont_info.continuation_column;
}

size_t file_highlighting_info::continue_column()
{
	return info.cont_info.continue_column;
}

//********************** highlighting_info ***********************

all_highlighting_info::all_highlighting_info(file_id * files, size_t files_count) :
	files_(files), files_count_(files_count) {}

file_id * all_highlighting_info::files()
{
	return files_;
}

size_t all_highlighting_info::files_count()
{
	return files_count_;
}

file_highlighting_info all_highlighting_info::file_info(file_id file_id)
{
	return file_id->get_hl_info();
}

//********************* diagnostics_container *******************

class diagnostic_list_impl
{
public:
	std::vector<diagnostic_s> diags;
};

diagnostic_list::diagnostic_list() : begin_(nullptr), size_(0)
{
}

diagnostic_list::diagnostic_list(diagnostic_s * begin, size_t size) : begin_(begin), size_(size) {}

diagnostic diagnostic_list::diagnostics(size_t index)
{
	return begin_[index];
}

size_t diagnostic_list::diagnostics_size()
{
	return size_;
}

position_uris::position_uris(semantics::position_uri_s * data, size_t size) : data_(data), size_(size) {}

position_uri position_uris::get_position_uri(size_t index)
{
	return data_[index];
}
size_t position_uris::size()
{
	return size_;
}

token_info::token_info(const range & token_range, semantics::hl_scopes scope) : token_range(token_range), scope(scope) {};
token_info::token_info(size_t line_start, size_t column_start, size_t line_end, size_t column_end, semantics::hl_scopes scope) : token_range({ {line_start, column_start}, {line_end, column_end} }), scope(scope) {};
//*********************** stack_frame *************************
stack_frame::stack_frame(const debugging::stack_frame & frame) : impl_(frame) {}

const char * stack_frame::name()
{
	return impl_.name.c_str();
}

uint32_t stack_frame::id()
{
	return impl_.id;
}

range stack_frame::get_range()
{
	return { {impl_.begin_line, 0}, {impl_.end_line, 0} };
}

source stack_frame::get_source()
{
	return impl_.frame_source;
}

template<>
stack_frame c_view_array<stack_frame, debugging::stack_frame>::item(size_t index)
{
	return data_[index];
}

//********************* source **********************

source::source(const debugging::source & source) : source_(source) {}

const char * source::path()
{
	return source_.path.c_str();
}

//*********************** scope *************************

scope::scope(const debugging::scope & impl) :
	impl_(impl) {}

const char * scope::name() const
{
	return impl_.name.c_str();
}

var_reference_t scope::variable_reference() const
{
	return impl_.var_reference;
}

source scope::get_source() const
{
	return impl_.scope_source;
}

template<>
scope c_view_array<scope, debugging::scope>::item(size_t index)
{
	return data_[index];
}


//********************** variable **********************

variable::variable(const debugging::variable & impl) : impl_(impl) {}

const char * variable::name() const
{
	return impl_.get_name().c_str();
}

set_type variable::type() const
{
	return impl_.type();
}

const char * variable::value() const
{
	return impl_.get_value().c_str();
}

var_reference_t variable::variable_reference() const
{
	return impl_.var_reference;
}

template<>
variable c_view_array<variable, debugging::variable *>::item(size_t index)
{
	return *data_[index];
}





}
