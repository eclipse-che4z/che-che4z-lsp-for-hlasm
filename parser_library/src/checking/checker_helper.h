#ifndef HLASMPLUGIN_PARSERLIBRRY_CHECKER_HELPER_H
#define HLASMPLUGIN_PARSERLIBRRY_CHECKER_HELPER_H

#include <string>
#include "instr_operand.h"
#include "../context/instruction.h"
#include <cassert>

#pragma warning(push)
#pragma warning(disable : 4505)
namespace hlasm_plugin
{
namespace parser_library
{
namespace checking
{

	static bool is_number(const std::string& to_test, bool must_be_positive = false) //is any negative or positive number
	{
		bool is_negative = false;
		std::string::const_iterator it = to_test.begin();
		if (it != to_test.end() && *it == '-')
		{
			if (must_be_positive)
				return false;
			it++;
			is_negative = true;
		}
		while (it != to_test.end() && isdigit(*it)) ++it;
		return !to_test.empty() && it == to_test.end() && (to_test.size() > 1 || !is_negative);
	}

	static bool is_positive_number(const std::string& to_test)
	{
		return (is_number(to_test,true) && to_test[0] != '-');
	}

	// function to convert numbers less than 64000 to hexadecimal
	static std::string dec_to_hexa(int to_convert)
	{
		if (to_convert >= 64000 || to_convert < 0) return "- 1";
		std::string result;
		do { 
			result.push_back("0123456789ABCDEF"[to_convert % 16]); 
		} while (to_convert /= 16);
		std::reverse(result.begin(), result.end());
		return result;
	}

	static bool is_value_hexa(const std::string& to_test)
	{
		return !to_test.empty() && std::all_of(to_test.cbegin(), to_test.cend(), ::isxdigit);
	}

	static bool is_positive_value(const std::string& to_test)
	{
		return is_number(to_test) && is_positive_number(to_test);
	}

	static bool is_byte_value(const std::string& to_test)
	{
		if (is_number(to_test))
		{
			int temp_int = std::stoi(to_test);
			if (temp_int <= 255 && temp_int >= 0)
				return true;
		}
		return false;
	}

	static bool is_power_of_two(int to_check)
	{
		if (to_check >= 0)
			return (to_check & (to_check - 1)) == 0;
		else
			return false;
	}

	static bool is_ord_symbol(const std::string& to_test)
	{
		assert(!to_test.empty());
		return !to_test.empty() && to_test.size() <= 63 && isalpha(to_test.front()) && std::all_of(to_test.cbegin(), to_test.cend(), ::isalnum);
	}

	static bool is_var_symbol(const std::string& to_test)
	{
		assert(!to_test.empty());
		return to_test.front() == '&' && is_ord_symbol(to_test.substr(1));
	}

	static bool is_base_register(const std::string& to_test)
	{
		try
		{
			int base_register = std::stoi(to_test);
			if (base_register > 15 || base_register < 0)
				return false;
		}
		catch (std::invalid_argument)
		{
			return false;
		}
		catch (std::out_of_range)
		{
			return false;
		}
		return true;
	}

	static bool is_character_string(const std::string& to_test)
	{
		return !(to_test.size() == 1 || to_test.front() != '\'' || to_test.back() != '\'' || to_test.size() > 255);
	}

	static bool is_operand_complex(const  one_operand* to_check_operand)
	{
		return !!dynamic_cast<const complex_operand*> (to_check_operand);
	}

	static bool is_operand_address(const one_operand* to_check_operand)
	{
		return dynamic_cast<const address_operand*> (to_check_operand) != NULL;
	}

	static bool is_operand_simple(const one_operand* to_check_operand)
	{
		return (!is_operand_address(to_check_operand) && !is_operand_complex(to_check_operand));
	}

	static bool is_date(const std::string& to_test)
	{
		const static size_t days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
		if (to_test.size() != 8)
			return false;
		int year, month, day;
		try
		{
			year = std::stoi(to_test.substr(0, 4));
			month = std::stoi(to_test.substr(4, 2));
			day = std::stoi(to_test.substr(6, 2));
		}
		catch (...)
		{
			return false;
		}
		if (year < 0 || month < 1 || month > 12 || day > 31 || day < 1)
			return false;
		bool leap_year = true;
		if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
			leap_year = true;
		else
			leap_year = false;

		return ((size_t)day <= days_in_month[month-1] || (month == 2 && leap_year && day == 29));
	};

	static int get_size_of_operand(context::operand_format format)
	{
		switch (format)
		{
			case context::operand_format::SIZE_4b:
					return 4;
			case context::operand_format::SIZE_8b:
					return 8;
			case context::operand_format::SIZE_12b:
					return 12;
			case context::operand_format::SIZE_16b:
					return 16;
			case context::operand_format::SIZE_24b:
					return 24;
			case context::operand_format::SIZE_32b:
					return 32;
			default:
				return -1;
		}	
	}

	static bool is_size_corresponding_int(int size_of_operand, int to_check)
	{
		return to_check >= 0 && to_check <= (size_of_operand*size_of_operand) - 1;
	}

	static bool is_size_corresponding_str(int size_of_operand, const std::string & to_check)
	{
		return is_positive_number(to_check) && is_size_corresponding_int(size_of_operand, std::stoi(to_check));
	}

	static bool is_displacement_operand(context::operand_format format)
	{
		switch (format)
		{
			case context::operand_format::DB:
			case context::operand_format::DXB_8b:
			case context::operand_format::DXB_4b:
				return true;
			default:
				return false;
		}
	}

}
}
}

#pragma warning(pop)

#endif // !HLASMPLUGIN_PARSERLIBRRY_CHECKER_HELPER_H
