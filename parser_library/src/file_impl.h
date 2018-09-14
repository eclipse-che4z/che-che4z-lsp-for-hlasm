#ifndef HLASMPLUGIN_PARSERLIBRARY_FILE_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_FILE_IMPL_H

#include "file.h"

namespace hlasm_plugin::parser_library {

class file_impl : public file
{
public:
	explicit file_impl(file_uri uri);
	file_impl(const file_impl &) = delete;
	file_impl & operator= (const file_impl &) = delete;

	file_impl(file_impl &&) = default;
	file_impl & operator= (file_impl &&) = default;


	virtual const file_uri & get_file_name() override;
	virtual const std::string & get_text() override;
	virtual version_t get_version() override;
	virtual bool is_bad() const override;

	virtual void did_open(std::string new_text, version_t version) override;
	virtual void did_change(std::string new_text) override;
	virtual void did_change(range range, std::string new_text) override;
	virtual void did_close() override;
private:
	file_uri file_name_;
	std::string text_;
	std::vector<size_t> lines_ind_;

	bool up_to_date_ = false;
	bool editing_ = false;
	bool bad_ = false;

	version_t version_ = 0;

	void load_text();

	size_t index_from_location(location location);
};

}

#endif // !HLASMPLUGIN_PARSERLIBRARY_FILE_IMPL_H
