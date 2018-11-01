
#include <fstream>
#include <cerrno>
#include <string>
#include <locale>
#include <codecvt>
#include <exception>

#include "file_impl.h"


namespace hlasm_plugin {
namespace parser_library {

file_impl::file_impl(file_uri uri) : file_name_(std::move(uri))
{
}

const file_uri & file_impl::get_file_name()
{
	return file_name_;
}

const std::string & file_impl::get_text()
{
	if (!up_to_date_)
		load_text();
	return text_;
}

void file_impl::load_text()
{
	std::ifstream fin(file_name_, std::ios::in | std::ios::binary);

	if(fin)
	{
		fin.seekg(0, std::ios::end);
		text_.resize((size_t)fin.tellg());
		fin.seekg(0, std::ios::beg);
		fin.read(&text_[0], text_.size());
		fin.close();

		up_to_date_ = true;
		bad_ = false;
		return;
	}
	else
	{
		text_ = "";
		up_to_date_ = true;
		bad_ = true;
		//TODO: publish diagnostic warning that we are unable to read from the file
	}
}

//adds positions of newlines into vector 'lines'
size_t find_newlines(const std::string & text, std::vector<size_t> & lines)
{
	size_t before = lines.size();
	bool was_r = false;
	for (size_t i = 0; i < text.size(); ++i)
	{
		char ch = text[i];
		if (was_r)
		{
			if (ch == '\n')
			{
				lines.push_back(i + 1);
				was_r = false;
			}
			else if (ch == '\r')
				lines.push_back(i);
			else
			{
				lines.push_back(i);
				was_r = false;
			}
		}
		else
		{
			if (ch == '\n')
				lines.push_back(i + 1);
			else if (ch == '\r')
				was_r = true;
		}
	}

	if (was_r)
		lines.push_back(text.size());

	return lines.size() - before;
}

void file_impl::did_open(std::string new_text, version_t version)
{
	text_ = std::move(new_text);
	version_ = version;

	lines_ind_.clear();
	lines_ind_.push_back(0);

	find_newlines(text_, lines_ind_);
	up_to_date_ = true;
	bad_ = false;
}

//applies a change to the text and updates line begginings 
void file_impl::did_change(range range, std::string new_text)
{
	size_t range_end_line = (size_t)range.end.line;
	size_t range_start_line = (size_t)range.start.line;
	
	size_t begin = index_from_location(range.start);
	size_t end = index_from_location(range.end);

	text_.replace(begin, end - begin, new_text);

	std::vector<size_t> new_lines;
	
	find_newlines(new_text, new_lines);

	
	size_t old_lines_count = range_end_line - range_start_line;
	size_t new_lines_count = new_lines.size();

	size_t char_diff = new_text.size() - (end - begin);

	//add or remove lines depending on the difference
	if (new_lines_count > old_lines_count)
	{
		size_t diff = new_lines_count - old_lines_count;
		lines_ind_.insert(lines_ind_.end(), diff, 0);

		for (size_t i = lines_ind_.size() - 1; i > range_end_line; --i)
		{
			lines_ind_[i] = lines_ind_[i - diff] + char_diff;
		}
	}
	else
	{
		size_t diff = old_lines_count - new_lines_count;
		
		for (size_t i = range_start_line + 1 + new_lines_count; i < lines_ind_.size() - diff; ++i)
		{
			lines_ind_[i] = lines_ind_[i + diff] + char_diff;
		}
		
		for (size_t i = 0; i < diff; ++i)
			lines_ind_.pop_back();
	}


	for (size_t i = range_start_line+1; i <= range_start_line + new_lines_count; ++i)
	{
		lines_ind_[i] = new_lines[i - (size_t) range_start_line - 1] + begin;
	}

	
	++version_;
}

void file_impl::did_change(std::string new_text)
{
	text_ = std::move(new_text);
	lines_ind_.clear();
	lines_ind_.push_back(0);
	find_newlines(text_, lines_ind_);
	++version_;
}

void file_impl::did_close()
{}

version_t file_impl::get_version()
{
	return version_;
}

bool file_impl::is_bad() const
{
	return bad_;
}

//returns the location in text_ that corresponds to utf-16 based location
size_t file_impl::index_from_location(location loc)
{
	size_t end = (size_t)loc.column;
	size_t i = lines_ind_[(size_t)loc.line];
	size_t utf16_counter = 0;

	while(utf16_counter < end)
	{
		if (text_[i] & 0x80)
		{
			char width;
			char utf16_width;
			if ((text_[i] & 0xF0) == 0xF0 && (text_[i] & 0x08) == 0) //11110xxx
			{
				width = 4;
				utf16_width = 2;
			}
			else if ((text_[i] & 0xE0) == 0xE0 && (text_[i] & 0x10) == 0) //1110xxxx
			{
				width = 3;
				utf16_width = 1;
			}
			else if ((text_[i] & 0xA0) == 0xA0 && (text_[i] & 0x20) == 0) //110xxxxx
			{
				width = 2;
				utf16_width = 1;
			}
			else
				throw std::runtime_error("The text of the file is not in utf-8.");//WRONG UTF-8 input

			i += width;
			utf16_counter += utf16_width;
		}
		else
		{
			++i;
			++utf16_counter;
		}
	}
	return i;
}

}
}
