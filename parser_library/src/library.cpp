#include "library.h"

#include <filesystem>

namespace hlasm_plugin::parser_library {


library_local::library_local(file_manager & file_manager, std::string lib_path) : file_manager_(file_manager), lib_path_(lib_path) {}

library_local::library_local(library_local && l) : file_manager_(l.file_manager_) {}

void library_local::collect_diags() const
{

}

const std::string & library_local::get_lib_path() const
{
	return lib_path_;
}

processor * library_local::find_file(const std::string & file_name)
{
	std::filesystem::path lib_path(lib_path_);
	std::filesystem::path file_path(lib_path / file_name);
	if (file_manager_.lib_file_exists(lib_path_, file_name))
	{
		return file_manager_.add_processor_file(file_path.string());
	}
	else
	{
		return nullptr;
		//diag?
	}
}

}
