#include <map>

#include "file_manager_impl.h"
#include "processor_file_impl.h"

namespace hlasm_plugin {
namespace parser_library {

void file_manager_impl::collect_diags() const
{
	for (auto & it : files_)
	{
		collect_diags_from_child(*it.second);
	}
}

file * file_manager_impl::add_file(const file_uri & uri)
{
	auto ret = files_.emplace(uri, std::make_unique<file_impl>(uri));
	return ret.first->second.get();
}

processor_file * file_manager_impl::change_into_processor_file_if_not_already_(std::unique_ptr<file_impl>& ret)
{
	auto processor = dynamic_cast<processor_file *>(ret.get());
	if (processor)
		return processor;
	else
	{
		auto processor_unique_ptr = std::make_unique<processor_file_impl>(std::move(*ret), cancel_);
		auto processor_ptr = processor_unique_ptr.get();
		ret = std::move(processor_unique_ptr);
		return processor_ptr;
	}
}

processor_file * file_manager_impl::add_processor_file(const file_uri & uri)
{
	auto ret = files_.find(uri);
	if (ret == files_.end())
	{
		auto unique = std::make_unique<processor_file_impl>(uri, cancel_);
		auto ptr = unique.get();
		files_.emplace(uri, std::move(unique));
		return ptr;
	}
	else
	{
		return change_into_processor_file_if_not_already_(ret->second);
	}
}

void file_manager_impl::remove_file(const file_uri& document_uri)
{
	auto file = files_.find(document_uri);
	if (file == files_.end())
		return;

	//close the file internally
	files_.erase(document_uri);
}

file * file_manager_impl::find(const std::string & key)
{
	auto ret = files_.find(key);
	if (ret == files_.end())
		return nullptr;

	return ret->second.get();
}

processor_file * file_manager_impl::find_processor_file(const std::string & key)
{
	auto ret = files_.find(key);
	if(ret == files_.end())
		return nullptr;

	return change_into_processor_file_if_not_already_(ret->second);
}

std::vector<file*> file_manager_impl::list_files()
{
	std::vector<file*> list;
	for (auto & item : files_)
	{
		list.push_back(item.second.get());
	}

	return list;
}

std::vector<processor_file *> file_manager_impl::list_processor_files()
{
	std::vector<processor_file *> list;
	for (auto & item : files_)
	{
		auto p = dynamic_cast<processor_file *>(item.second.get());
		if (p == nullptr)
			continue;
		list.push_back(p);
	}
	auto p = files_.find("");

	return list;
}


std::vector<processor_file *> file_manager_impl::list_updated_files()
{
	std::vector<processor_file *> list;
	for (auto & item : files_)
	{
		processor_file * pf = dynamic_cast<processor_file *>(item.second.get());
		if (pf && pf->parse_info_updated())
			list.push_back(pf);
	}
	auto p = files_.find("");

	return list;
}

std::unordered_set<std::string> file_manager_impl::list_directory_files(const std::string& path)
{
	std::filesystem::path lib_p(path);
	std::unordered_set<std::string> found_files;
	try {
		std::filesystem::directory_entry dir(lib_p);
		if (!dir.is_directory())
		{
			add_diagnostic(diagnostic_s{ "",{},"L0001", "Unable to load library: " + path + ". Error: The path does not point to directory." });
			return found_files;
		}

		std::filesystem::directory_iterator it(lib_p);

		for (auto& p : it)
		{
			if (p.is_regular_file())
				found_files.insert(p.path().filename().string());
		}

	}
	catch (const std::filesystem::filesystem_error & e)
	{
		add_diagnostic(diagnostic_s{ path ,{},"L0001", "Unable to load library: " + path + ". Error: " + e.what() });
	}
	return found_files;
}

void file_manager_impl::did_open_file(const std::string & document_uri, version_t version, std::string text)
{
	auto file = add_file(document_uri);
	file->did_open(std::move(text), version);
}

void file_manager_impl::did_change_file(const std::string & document_uri, version_t, const document_change * changes, size_t ch_size)
{
	//the version is the version after the changes -> I dont see how is that useful
	//should we just overwrite the version??
	//on the other hand, the spec clearly specifies that each change increments version by one.
	auto file = files_.find(document_uri);
	if (file == files_.end())
		return; //if the file does not exist, no action is taken

	for (size_t i = 0; i < ch_size; ++i)
	{
		std::string text_s(changes[i].text, changes[i].text_length);
		if (changes[i].whole)
			file->second->did_change(std::move(text_s));
		else
			file->second->did_change(changes[i].change_range, std::move(text_s));
	}
			
		
}

void file_manager_impl::did_close_file(const std::string & document_uri)
{
	auto file = files_.find(document_uri);
	if (file == files_.end())
		return;

	//close the file externally
	file->second->did_close();

	//if the file does not exist, no action is taken
}

bool file_manager_impl::file_exists(const std::string & file_name)
{
	std::error_code ec;
	return std::filesystem::exists(file_name, ec);
	//TODO use error code??
}

bool file_manager_impl::lib_file_exists(const std::string & lib_path, const std::string & file_name)
{
	std::filesystem::path lib_path_p(lib_path);
	std::filesystem::path file_path(lib_path_p / file_name);
	return std::filesystem::exists(file_path);
}


}
}
