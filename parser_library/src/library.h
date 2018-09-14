#ifndef HLASMPLUGIN_PARSERLIBRARY_LIBRARY_H
#define HLASMPLUGIN_PARSERLIBRARY_LIBRARY_H

#include <string>
#include <unordered_map>

#include "file_manager.h"


namespace hlasm_plugin::parser_library {

class library
{
public:
	virtual file * find_file(std::string file) = 0;

	virtual ~library() = default;
private:
};

//library holds absolute path to a directory and list of files it has opened so far
class library_local : public library
{
public:
	library_local(file_manager& file_manager, std::string lib_path);
	
	library_local(const library_local &) = delete;
	library_local & operator= (const library_local &) = delete;

	library_local(library_local && l);

	const std::string & get_lib_path() const;

	virtual file * find_file(std::string file) override;
private:
	file_manager & file_manager_;
	std::unordered_map<std::string, file *> files_;
	std::string lib_path_;
};

}
#endif
