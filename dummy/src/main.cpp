#include <iostream>


#include <memory>
#include <string>
#include <thread>

#include <fstream>
#include <cstdio>
#include <sstream>

#include <shared/parser_library.h>

int main(int argc, char *argv[])
{

#ifdef CONSOLE

	std::string input;

	while (std::getline(std::cin, input))
	{
		auto p = new hlasm_plugin::parser_library::parser_library();
		p->parse(std::move(input));
	}

#else

	//std::string tcase = "simple";
	//std::string tcase = "operand";
	//std::string tcase = "continuation";
	//std::string tcase = "model_statement";
	//std::string tcase = "correctness";
	//std::string tcase = "aread";
	//std::string tcase = "comment";
	std::string tcase = "test";
	std::string inp = "test/library/input/"+ tcase +".in";
	if (argc > 1)
		inp = std::string(argv[1]);
	std::ifstream ifs(inp);
	std::string content(std::istreambuf_iterator<char>(ifs), (std::istreambuf_iterator<char>()));

	auto p = new hlasm_plugin::parser_library::parser_library();
	p->parse(std::move(content));

#endif

	return 0;
}