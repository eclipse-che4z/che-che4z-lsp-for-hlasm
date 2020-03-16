#include "wildcard.h"
#include <cassert>

static const std::regex escape("(\\(|\\[|\\{|\\\\|\\^|\\-|\\=|\\$|\\!|\\||\\]|\\}|\\)|\\.)");
static const std::regex question("\\?");
static const std::regex slash("\\/");
static const std::regex nongreedy("(\\*|\\+)");

std::regex hlasm_plugin::parser_library::wildcard2regex(const std::string& wildcard)
{
	auto regex_str = wildcard;
#ifdef _WIN32
	// change of forward slash to double backslash on windows
	regex_str = std::regex_replace(regex_str, slash, "\\");
#endif
	regex_str = std::regex_replace(regex_str, escape, "\\$1");
	regex_str = std::regex_replace(regex_str, question, ".");
	regex_str = std::regex_replace(regex_str, nongreedy, ".$1?");
	return	std::regex(regex_str);
}
