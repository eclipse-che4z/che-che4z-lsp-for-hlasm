#ifndef HLASMPLUGIN_PARSERLIBRARY_LIBRARY_H
#define HLASMPLUGIN_PARSERLIBRARY_LIBRARY_H

#include <string>
#include <unordered_map>

#include "file_manager.h"
#include "diagnosable_impl.h"

namespace hlasm_plugin::parser_library {

class library : public virtual diagnosable
{
public:
	virtual processor * find_file(const std::string & file) = 0;
private:
};

#pragma warning(push)
#pragma warning(disable: 4250)

//library holds absolute path to a directory and list of files it has opened so far
class library_local : public library, public diagnosable_impl
{
public:
	library_local(file_manager& file_manager, std::string lib_path);
	
	library_local(const library_local &) = delete;
	library_local & operator= (const library_local &) = delete;

	library_local(library_local && l);

	void collect_diags() const override;

	const std::string & get_lib_path() const;

	virtual processor * find_file(const std::string & file) override;
private:
	file_manager & file_manager_;
	//std::unordered_map<std::string, file *> files_;
	std::string lib_path_;
};
#pragma warning(pop)

}
#endif
