#include <iostream>

#include "shared/HlasmParserLibrary.h"
#include "antlr4-runtime.h"
#include "generated/HlasmLexer.h"

namespace HlasmPlugin{
namespace HlasmParserLibrary {

void HlasmParserLibrary::parse(std::string && src)
{
	antlr4::ANTLRInputStream input(src);
	HlasmGenerated::HlasmLexer lexer(&input);

	antlr4::CommonTokenStream tokens(&lexer);

	tokens.fill();

	for (auto && token : tokens.getTokens())
		std::cout << token->toString() << std::endl;

	std::cin.get();
}

} //namespace HlasmParserLibrary
} //namespace HlasmPlugin