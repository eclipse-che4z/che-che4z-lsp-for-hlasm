/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H

#include <memory>

#include "file_manager.h"
#include "processor_file_impl.h"
#include "diagnosable_impl.h"

namespace hlasm_plugin::parser_library {

#pragma warning(push)
#pragma warning(disable : 4250)

class file_manager_impl : public file_manager, public diagnosable_impl
{
public:
	file_manager_impl(std::atomic<bool>* cancel = nullptr) : cancel_(cancel) {};
	file_manager_impl(const file_manager_impl &) = delete;
	file_manager_impl & operator=(const file_manager_impl &) = delete;

	file_manager_impl(file_manager_impl &&) = default;
	file_manager_impl & operator=(file_manager_impl &&) = default;

	virtual void collect_diags() const override;

	virtual file_ptr add_file(const file_uri &) override;
	virtual processor_file_ptr add_processor_file(const file_uri &) override;
	virtual void remove_file(const file_uri&) override;

	virtual file_ptr find(const std::string & key) override;
	virtual processor_file_ptr find_processor_file(const std::string & key) override;

	virtual std::vector<processor_file *> list_updated_files();
	virtual std::unordered_set<std::string> list_directory_files(const std::string& path) override;

	virtual void did_open_file(const std::string & document_uri, version_t version, std::string text) override;
	virtual void did_change_file(const std::string & document_uri, version_t version, const document_change * changes, size_t ch_size) override;
	virtual void did_close_file(const std::string & document_uri) override;
	virtual bool file_exists(const std::string & file_name) override;
	virtual bool lib_file_exists(const std::string & lib_path, const std::string & file_name) override;

	virtual ~file_manager_impl() = default;
protected:
	std::unordered_map <std::string, std::shared_ptr<file_impl>> files_;

private:
	std::mutex files_mutex;

	std::atomic<bool>* cancel_;

	processor_file_ptr change_into_processor_file_if_not_already_(std::shared_ptr<file_impl>& ret);
	void prepare_file_for_change_(std::shared_ptr<file_impl>& file);
};

#pragma warning(pop)

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_FILE_MANAGER_IMPL_H
