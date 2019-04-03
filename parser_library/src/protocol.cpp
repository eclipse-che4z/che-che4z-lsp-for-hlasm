#include "shared/protocol.h"
#include "diagnosable.h"
#include "semantics/semantic_highlighting_info.h"
#include "semantics/semantic_info.h"
#include "processor.h"

namespace hlasm_plugin::parser_library
{

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
file_semantic_info::file_semantic_info(semantics::semantic_info & info) :impl(info) {}

const char * file_semantic_info::document_uri()
{
	return impl.hl_info.document.uri.c_str();
}

version_t file_semantic_info::document_version()
{
	return impl.hl_info.document.version;
}

token_info file_semantic_info::token(size_t index)
{
	return impl.hl_info.lines[index];
}

size_t file_semantic_info::token_count()
{
	return impl.hl_info.lines.size();
}

position file_semantic_info::continuation(size_t index)
{
	return impl.continuation_positions[index];
}

size_t file_semantic_info::continuation_count()
{
	return impl.continuation_positions.size();
}

size_t file_semantic_info::continuation_column()
{
	return impl.continuation_column;
}

size_t file_semantic_info::continue_column()
{
	return impl.continue_column;
}

//********************** highlighting_info ***********************

all_semantic_info::all_semantic_info(file_id * files, size_t files_count) :
	files_(files), files_count_(files_count) {}

file_id * all_semantic_info::files()
{
	return files_;
}

size_t all_semantic_info::files_count()
{
	return files_count_;
}

file_semantic_info all_semantic_info::file_info(file_id file_id)
{
	return file_id->semantic_info();
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

token_info::token_info(const semantics::symbol_range & range, semantics::scopes scope) : token_range({ {range.begin_ln, range.begin_col}, {range.end_ln, range.end_col} }), scope(scope) {}

}
