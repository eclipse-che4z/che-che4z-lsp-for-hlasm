#include "library.h"

#include <filesystem>
#include <locale>
namespace hlasm_plugin::parser_library {

library_local::library_local(file_manager& file_manager, std::string lib_path) : file_manager_(file_manager), lib_path_(lib_path) {}

library_local::library_local(library_local&& l) : file_manager_(l.file_manager_) {}

void library_local::collect_diags() const
{

}

void library_local::refresh()
{
	files_.clear();
	load_files();
}

const std::string& library_local::get_lib_path() const
{
	return lib_path_;
}

processor* library_local::find_file(const std::string& file_name)
{
	if (!files_loaded_)
		load_files();

	auto found = files_.find(file_name);
	if (found != files_.end())
	{
		std::filesystem::path lib_path(lib_path_);
		return file_manager_.add_processor_file((lib_path / file_name).string());
	}
	else
		return nullptr;
}



void library_local::load_files()
{
	files_ = file_manager_.list_directory_files(lib_path_);
	files_loaded_ = true;
}

}
