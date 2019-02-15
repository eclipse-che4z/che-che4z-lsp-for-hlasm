#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H

#include "file_manager.h"
#include "processor_file_impl.h"
#include "diagnosable_impl.h"

namespace hlasm_plugin::parser_library {

#pragma warning(push)
#pragma warning(disable : 4250)

class file_manager_impl : public file_manager, public diagnosable_impl
{
public:
	file_manager_impl() {};
	file_manager_impl(const file_manager_impl &) = delete;
	file_manager_impl & operator=(const file_manager_impl &) = delete;

	file_manager_impl(file_manager_impl &&) = default;
	file_manager_impl & operator=(file_manager_impl &&) = default;

	virtual void collect_diags() const override;

	virtual file * add_file(const file_uri &) override;
	virtual processor_file * add_processor_file(const file_uri &) override;

	virtual file * find(const std::string & key) override;
	virtual processor_file * find_processor_file(const std::string & key) override;

	virtual std::vector<file *> list_files() override;
	virtual std::vector<processor_file *> list_processor_files();

	virtual std::vector<processor_file *> list_updated_files() override;

	virtual void did_open_file(const std::string & document_uri, version_t version, std::string text) override;
	virtual void did_change_file(const std::string & document_uri, version_t version, const document_change * changes, size_t ch_size) override;
	virtual void did_close_file(const std::string & document_uri) override;

	virtual bool file_exists(const std::string & file_name) override;
	virtual bool lib_file_exists(const std::string & lib_path, const std::string & file_name) override;

	virtual ~file_manager_impl() = default;
private:
	std::unordered_map <std::string, std::unique_ptr<file_impl>> files_;
};

#pragma warning(pop)

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H
