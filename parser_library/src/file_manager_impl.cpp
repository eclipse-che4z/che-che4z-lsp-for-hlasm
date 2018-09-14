#include <map>

#include "file_manager_impl.h"


namespace hlasm_plugin {
namespace parser_library {

	file & file_manager_impl::add(const file_uri & uri)
	{
		auto ret = files_.emplace(uri, file_impl{ uri });
		return ret.first->second;
	}

	void file_manager_impl::did_open_file(const std::string & document_uri, version_t version, std::string text)
	{
		auto tmp = files_.emplace(document_uri, document_uri);
		tmp.first->second.did_open(std::move(text), version);
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
				file->second.did_change(std::move(text_s));
			else
				file->second.did_change(changes[i].range, std::move(text_s));
		}
			
		
	}

	void file_manager_impl::did_close_file(const std::string & document_uri)
	{
		auto file = files_.find(document_uri);
		if (file == files_.end())
			return;

		file->second.did_close();
	
		//if the file does not exist, no action is taken
	}

}
}
