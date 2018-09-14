#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H

#include "file_manager.h"
#include "file_impl.h"

namespace hlasm_plugin::parser_library {

class file_manager_impl : public file_manager
{
public:
	file_manager_impl() {};
	file_manager_impl(const file_manager_impl &) = delete;
	file_manager_impl & operator=(const file_manager_impl &) = delete;

	file_manager_impl(file_manager_impl &&) = default;
	file_manager_impl & operator=(file_manager_impl &&) = default;

	virtual file & add(const file_uri &) override;

	virtual void did_open_file(const std::string & document_uri, version_t version, std::string text) override;
	virtual void did_change_file(const std::string & document_uri, version_t version, const document_change * changes, size_t ch_size) override;
	virtual void did_close_file(const std::string & document_uri) override;

	virtual ~file_manager_impl() = default;
private:
	std::unordered_map <std::string, file_impl> files_;
};

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H
