#include <iostream>


#include <memory>
#include <string>
#include <thread>

#include <fstream>
#include <cstdio>
#include <sstream>

#include <shared/parser_library.h>
//#define CONSOLE

int main(int argc, char *argv[]) {
	using namespace std;
#ifdef CONSOLE
	std::string input;

	while (std::getline(std::cin, input))
	{
		auto p = new HlasmPlugin::HlasmParserLibrary::HlasmParserLibrary();
		p->parse(std::move(input));
	}

#else
	std::string inp = "input.txt";
	//std::string inp = "C:/Users/hruma02/Desktop/HlasmPlugin/tests/empty_continuations.in";
	//std::string inp = "C:/Users/hruma02/Desktop/HlasmPlugin/tests/continuation.in";
	//std::string inp = "C:/Users/hruma02/Desktop/HlasmPlugin/tests/model_statement.in";
	//std::string inp = "C:/Users/hruma02/Desktop/HlasmPlugin/tests/operand.in";
	if (argc > 1)
		inp = std::string(argv[1]);
	std::ifstream ifs(inp);
	std::string content( (std::istreambuf_iterator<char>(ifs) ),
	(std::istreambuf_iterator<char>()    ) );

	auto p = new HlasmPlugin::HlasmParserLibrary::HlasmParserLibrary();
	p->parse(std::move(content));
	std::cin.get();
#endif
	return 0;
}
