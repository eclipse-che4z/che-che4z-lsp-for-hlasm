#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_H

#include <string>
#include <unordered_map>
#include <filesystem>

#include "file.h"
#include "../src/generated/parser_library_export.h"

namespace hlasm_plugin {
namespace parser_library {

class file_manager
{
public:
	virtual file & add(const file_uri &) = 0;
	
	virtual void did_open_file(const std::string & document_uri, version_t version, std::string text) = 0;
	virtual void did_change_file(const std::string & document_uri, version_t version, const document_change * changes, size_t ch_size) = 0;
	virtual void did_close_file(const std::string & document_uri) = 0;
};

}
}
#endif // FILE_MANAGER_H
