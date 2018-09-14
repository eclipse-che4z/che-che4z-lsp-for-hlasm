#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_H

#include <string>
#include <istream>
#include <filesystem>

#include "../src/generated/parser_library_export.h"
#include "shared/protocol.h"

namespace hlasm_plugin {
namespace parser_library {

using file_uri = std::string;

class file
{
public:
	virtual const file_uri & get_file_name() = 0;
	virtual const std::string & get_text() = 0;
	virtual bool is_bad() const = 0;

	virtual version_t get_version() = 0;

	virtual void did_open(std::string new_text, version_t version) = 0;
	virtual void did_change(std::string new_text) = 0;
	virtual void did_change(range range, std::string new_text) = 0;
	virtual void did_close() = 0;
};





}
}

#endif
