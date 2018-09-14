#include <set>


#include "shared/workspace_manager.h"
#include "workspace.h"
#include "file_manager_impl.h"

namespace hlasm_plugin {
namespace parser_library {

class workspace_manager::impl
{
public:
	impl() {};
	impl(const impl &) = delete;
	impl & operator= (const impl &) = delete;

	impl(impl &&) = default;
	impl & operator= (impl &&) = default;

	size_t get_workspaces(ws_id * workspaces, size_t max_size)
	{
		size_t size = 0;
		
		for (auto it = workspaces_.begin(); size < max_size && it != workspaces_.end(); ++size, ++it)
		{
			workspaces[size] = &it->second;
		}
		return size;
	}

	size_t get_workspaces_count()
	{
		return workspaces_.size();
	}

	void add_workspace(const char * name, const char * uri)
	{
		workspaces_.emplace(name, workspace { uri, name, file_manager_ });
	}
	void remove_workspace(const char * uri)
	{
		workspaces_.erase(uri);
		//erase does no action, if the key does not exist
	}
	
	void did_open_file(const std::string & document_uri, version_t version, std::string text)
	{
		file_manager_.did_open_file(document_uri, version, std::move(text));
	}
	void did_change_file(const std::string document_uri, version_t version, const document_change * changes, size_t ch_size)
	{
		file_manager_.did_change_file(document_uri, version, changes, ch_size);
	}

	void did_close_file(const std::string document_uri)
	{
		file_manager_.did_close_file(document_uri);
	}
private:

	std::unordered_map<std::string, workspace> workspaces_;
	file_manager_impl file_manager_;
};

workspace_manager::workspace_manager() : impl_(new impl) {}

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


}
}
