#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_H

#include <string>
#include <unordered_map>
#include <filesystem>
#include <vector>

#include "file.h"
#include "diagnosable.h"
#include "processor.h"

namespace hlasm_plugin {
namespace parser_library {

class file_manager : public virtual diagnosable
{
public:
	virtual file * add_file(const file_uri &) = 0;
	virtual processor_file * add_processor_file(const file_uri &) = 0;

	virtual file * find(const std::string & key) = 0;
	virtual processor_file * find_processor_file(const std::string & key) = 0;

	virtual std::vector<file *> list_files() = 0;
	virtual std::vector<processor_file *> list_updated_files() = 0;

	virtual bool file_exists(const std::string & file_name) = 0;
	virtual bool lib_file_exists(const std::string & lib_path, const std::string & file_name) = 0;

	virtual void did_open_file(const std::string & document_uri, version_t version, std::string text) = 0;
	virtual void did_change_file(const std::string & document_uri, version_t version, const document_change * changes, size_t ch_size) = 0;
	virtual void did_close_file(const std::string & document_uri) = 0;

};

}
}
#endif // FILE_MANAGER_H
