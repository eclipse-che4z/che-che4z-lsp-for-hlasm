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

file_ptr file_manager_impl::add_file(const file_uri & uri)
{
	std::lock_guard guard(files_mutex);
	auto ret = files_.emplace(uri, std::make_shared<file_impl>(uri));
	return ret.first->second;
}

processor_file_ptr file_manager_impl::change_into_processor_file_if_not_already_(std::shared_ptr<file_impl>& to_change)
{
	auto processor = std::dynamic_pointer_cast<processor_file>(to_change);
	if (processor)
		return processor;
	else
	{
		auto proc_file = std::make_shared<processor_file_impl>(std::move(*to_change), cancel_);
		to_change = proc_file;
		return proc_file;
	}
}

processor_file_ptr file_manager_impl::add_processor_file(const file_uri & uri)
{
	std::lock_guard guard(files_mutex);
	auto ret = files_.find(uri);
	if (ret == files_.end())
	{
		auto ptr = std::make_shared<processor_file_impl>(uri, cancel_);
		files_.emplace(uri, ptr);
		return ptr;
	}
	else
		return change_into_processor_file_if_not_already_(ret->second);
}

void file_manager_impl::remove_file(const file_uri& document_uri)
{
	std::lock_guard guard(files_mutex);
	auto file = files_.find(document_uri);
	if (file == files_.end())
		return;

	//close the file internally
	files_.erase(document_uri);
}

file_ptr file_manager_impl::find(const std::string & key)
{
	std::lock_guard guard(files_mutex);
	auto ret = files_.find(key);
	if (ret == files_.end())
		return nullptr;

	return ret->second;
}

processor_file_ptr file_manager_impl::find_processor_file(const std::string & key)
{
	std::lock_guard guard(files_mutex);
	auto ret = files_.find(key);
	if(ret == files_.end())
		return nullptr;

	return change_into_processor_file_if_not_already_(ret->second);
}


std::vector<processor_file *> file_manager_impl::list_updated_files()
{
	std::vector<processor_file *> list;
	for (auto & item : files_)
	{
		auto p = dynamic_cast<processor_file *>(item.second.get());
		if (p && p->parse_info_updated())
			list.push_back(p);
	}
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

void file_manager_impl::prepare_file_for_change_(std::shared_ptr<file_impl>& file)
{
	if (file.unique())
		return;
	//another shared ptr to this file exists, we need to create a copy
	auto proc_file = std::dynamic_pointer_cast<processor_file>(file);
	if (proc_file)
		file = std::make_shared<processor_file_impl>(*file, cancel_);
	else
		file = std::make_shared<file_impl>(*file);
	
}

void file_manager_impl::did_open_file(const std::string & document_uri, version_t version, std::string text)
{
	std::lock_guard guard(files_mutex);
	auto ret = files_.emplace(document_uri, std::make_shared<file_impl>(document_uri));
	prepare_file_for_change_(ret.first->second);
	ret.first->second->did_open(std::move(text), version);
}

void file_manager_impl::did_change_file(const std::string & document_uri, version_t, const document_change * changes, size_t ch_size)
{
	//the version is the version after the changes -> I dont see how is that useful
	//should we just overwrite the version??
	//on the other hand, the spec clearly specifies that each change increments version by one.

	std::lock_guard guard(files_mutex);

	auto file = files_.find(document_uri);
	if (file == files_.end())
		return; //if the file does not exist, no action is taken

	prepare_file_for_change_(file->second);

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
	std::lock_guard guard(files_mutex);
	auto file = files_.find(document_uri);
	if (file == files_.end())
		return;

	prepare_file_for_change_(file->second);
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
