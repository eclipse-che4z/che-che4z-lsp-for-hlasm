#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_H

#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <fstream>

#include "../src/generated/parser_library_export.h"
#include "protocol.h"

namespace hlasm_plugin {
namespace parser_library {
class workspace;
using ws_id = workspace *;

class PARSER_LIBRARY_EXPORT workspace_manager
{
	class impl;
public:
	workspace_manager();

	workspace_manager(const workspace_manager &) = delete;
	workspace_manager& operator=(const workspace_manager &) = delete;

	workspace_manager(workspace_manager &&);
	workspace_manager& operator=(workspace_manager&&);

	virtual ~workspace_manager();

	
	virtual size_t get_workspaces(ws_id * workspaces, size_t max_size);
	virtual size_t get_workspaces_count();

	virtual void add_workspace(const char * name, const char * uri);
	virtual void remove_workspace(const char * uri);

	virtual void did_open_file(const char * document_uri, version_t version, const char * text, size_t text_size);
	virtual void did_change_file(const char * document_uri, version_t version, const document_change * changes, size_t ch_size);
	virtual void did_close_file(const char * document_uri);

private:
	impl * impl_;
};


}
}
#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_H
