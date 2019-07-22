#ifndef HLASMPLUGIN_PARSERLIBRARY_LIBRARY_H
#define HLASMPLUGIN_PARSERLIBRARY_LIBRARY_H

#include <string>
#include <unordered_map>
#include <unordered_set>

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

//library holds absolute path to a directory and finds macro files in it
class library_local : public library, public diagnosable_impl
{
public:
	//takes reference to file manager that provides access to the files
	//and normalised path to directory that it wraps.
	library_local(file_manager& file_manager, std::string lib_path);
	
	library_local(const library_local &) = delete;
	library_local & operator= (const library_local &) = delete;

	library_local(library_local && l);

	void collect_diags() const override;

	const std::string & get_lib_path() const;

	virtual processor * find_file(const std::string & file) override;

	//this function should be called from workspace, once watchedFilesChanged request is implemented
	void refresh();
private:
	file_manager & file_manager_;

	std::string lib_path_;
	std::unordered_set<std::string> files_;
	//indicates whether load_files function was called (not whether it was succesful)
	bool files_loaded_ = false;
	
	void load_files();
};
#pragma warning(pop)

}
#endif
