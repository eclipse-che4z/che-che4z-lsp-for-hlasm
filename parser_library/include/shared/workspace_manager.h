#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_H

#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <fstream>
#include <atomic>

#include "parser_library_export.h"
#include "protocol.h"

namespace hlasm_plugin {
namespace parser_library {
class workspace;
using ws_id = workspace *;

class PARSER_LIBRARY_EXPORT highlighting_consumer
{
public:
	virtual void consume_highlighting_info(all_highlighting_info info) = 0;
	virtual ~highlighting_consumer() {};
};

class PARSER_LIBRARY_EXPORT diagnostics_consumer
{
public:
	virtual void consume_diagnostics(diagnostic_list diagnostics) = 0;
	virtual ~diagnostics_consumer() {};
};

class PARSER_LIBRARY_EXPORT workspace_manager
{
	class impl;
public:
	workspace_manager(std::atomic<bool> * cancel = nullptr);

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
	virtual void did_change_watched_files(const char** paths, size_t size);

	position_uri definition(const char * document_uri, const position pos);
	position_uris references(const char * document_uri, const position pos);
	const string_array hover(const char * document_uri, const position pos);
	completion_list completion(const char* document_uri, const position pos, const char trigger_char, int trigger_kind);
	
	virtual void register_highlighting_consumer(highlighting_consumer * consumer);
	virtual void register_diagnostics_consumer(diagnostics_consumer * consumer);

private:
	impl * impl_;
};


}
}
#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_H
