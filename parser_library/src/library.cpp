#include "library.h"



namespace hlasm_plugin::parser_library {


library_local::library_local(file_manager & file_manager, std::string lib_path) : file_manager_(file_manager), lib_path_(lib_path) {}

library_local::library_local(library_local && l) : file_manager_(l.file_manager_) {}

const std::string & library_local::get_lib_path() const
{
	return lib_path_;
}

file * library_local::find_file(std::string file)
{
	//TODO
	return nullptr;
}

}
