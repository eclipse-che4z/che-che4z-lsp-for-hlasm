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

	inline const one_operand* get_simple_operand(const asm_operand* to_check_operand)
	{
		return dynamic_cast<const one_operand*> (to_check_operand);
	}

	inline const complex_operand* get_complex_operand(const asm_operand* to_check_operand)
	{
		return dynamic_cast<const complex_operand*> (to_check_operand);
	}

	inline bool is_operand_complex(const asm_operand* to_check_operand)
	{
		return get_complex_operand(to_check_operand) != nullptr;
	}

	inline bool is_operand_simple(const asm_operand* to_check_operand)
	{
		return get_simple_operand(to_check_operand) != nullptr;
	}

	inline static bool is_number(const std::string& to_test, bool must_be_positive = false) //is any negative or positive number
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

	inline static bool is_positive_number(const std::string& to_test)
	{
		return (is_number(to_test,true) && to_test[0] != '-');
	}

	// function to convert numbers less than 64000 to hexadecimal
	inline static std::string dec_to_hexa(int to_convert)
	{
		if (to_convert >= 64000 || to_convert < 0) return "- 1";
		std::string result;
		do { 
			result.push_back("0123456789ABCDEF"[to_convert % 16]); 
		} while (to_convert /= 16);
		std::reverse(result.begin(), result.end());
		return result;
	}

	inline static bool is_value_hexa(const std::string& to_test)
	{
		return !to_test.empty() && std::all_of(to_test.cbegin(), to_test.cend(), ::isxdigit);
	}

	inline static bool is_positive_value(const std::string& to_test)
	{
		return is_number(to_test) && is_positive_number(to_test);
	}

	inline static bool is_byte_value(const std::string& to_test)
	{
		if (is_number(to_test))
		{
			int temp_int = std::stoi(to_test);
			if (temp_int <= 255 && temp_int >= 0)
				return true;
		}
		return false;
	}

	inline static bool is_power_of_two(int to_check)
	{
		if (to_check >= 0)
			return (to_check & (to_check - 1)) == 0;
		else
			return false;
	}

	inline static bool is_ord_symbol(const std::string& to_test)
	{
		assert(!to_test.empty());
		return !to_test.empty() && to_test.size() <= 63 && isalpha(to_test.front()) && std::all_of(to_test.cbegin(), to_test.cend(), ::isalnum);
	}

	inline static bool is_var_symbol(const std::string& to_test)
	{
		assert(!to_test.empty());
		return to_test.front() == '&' && is_ord_symbol(to_test.substr(1));
	}

	inline static bool is_base_register(const std::string& to_test)
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

	inline static bool is_character_string(const std::string& to_test)
	{
		return to_test.empty() || !(to_test.size() == 1 || to_test.front() != '\'' || to_test.back() != '\'' || to_test.size() > 255);
	}

	inline static bool is_date(const std::string& to_test)
	{
		const static size_t days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
		if (to_test.size() != 8 || !is_positive_number(to_test))
			return false;
		size_t year, month, day;
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
		if (month < 1 || month > 12 || day > 31 || day < 1)
			return false;
		bool leap_year = true;
		if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
			leap_year = true;
		else
			leap_year = false;
		return ((size_t)day <= days_in_month[month-1] || (month == 2 && leap_year && day == 29));
	};
}
}
}

#pragma warning(pop)

#endif // !HLASMPLUGIN_PARSERLIBRRY_CHECKER_HELPER_H
