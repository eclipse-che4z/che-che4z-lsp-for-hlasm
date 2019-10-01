#include <set>
#include <string>
#include <vector>

#include "shared/workspace_manager.h"
#include "workspace.h"
#include "file_manager_impl.h"
#include "workspace_manager_impl.h"

namespace hlasm_plugin {
namespace parser_library {


workspace_manager::workspace_manager(std::atomic<bool>* cancel) : impl_(new impl(cancel)) {}

workspace_manager::workspace_manager(workspace_manager && ws_mngr) : impl_(ws_mngr.impl_)
{
	ws_mngr.impl_ = nullptr;
}
workspace_manager& workspace_manager::operator=(workspace_manager&& ws_mngr) 
{
	std::swap(impl_, ws_mngr.impl_);
	return *this;
}

workspace_manager::~workspace_manager()
{
	delete impl_;
}

size_t workspace_manager::get_workspaces(ws_id * workspaces, size_t max_size)
{
	return impl_->get_workspaces(workspaces, max_size);
}

size_t workspace_manager::get_workspaces_count()
{
	return impl_->get_workspaces_count();
}

void workspace_manager::add_workspace(const char * name, const char * uri)
{
	impl_->add_workspace(name, uri);
}

void workspace_manager::remove_workspace(const char * uri)
{
	impl_->remove_workspace(uri);
}

void workspace_manager::did_change_watched_files(const char** paths, size_t size)
{
	std::vector<std::string> paths_s;
	for (size_t i = 0; i < size; ++i)
	{
		paths_s.push_back(paths[i]);
	}
	impl_->did_change_watched_files(std::move(paths_s));
}

void workspace_manager::did_open_file(const char * document_uri, version_t version, const char * text, size_t text_size)
{
	impl_->did_open_file(document_uri, version, std::string( text, text_size ));
}
void workspace_manager::did_change_file(const char * document_uri, version_t version, const document_change * changes, size_t ch_size)
{
	impl_->did_change_file(document_uri, version, changes, ch_size);
}

void workspace_manager::did_close_file(const char * document_uri)
{
	impl_->did_close_file(document_uri);
}

void workspace_manager::register_highlighting_consumer(highlighting_consumer * consumer)
{
	impl_->register_highlighting_consumer(consumer);
}

void workspace_manager::register_diagnostics_consumer(diagnostics_consumer * consumer)
{
	impl_->register_diagnostics_consumer(consumer);
}


position_uri workspace_manager::definition(const char * document_uri, const position pos)
{
	return impl_->definition(document_uri, pos);
}

position_uris workspace_manager::references(const char * document_uri, const position pos)
{
	return impl_->references(document_uri, pos);
}

const string_array workspace_manager::hover(const char * document_uri, const position pos)
{
	return impl_->hover(document_uri, pos);
}

completion_list workspace_manager::completion(const char* document_uri, const position pos, const char trigger_char, int trigger_kind)
{
	return impl_->completion(document_uri, pos, trigger_char, trigger_kind);
}

}
}
