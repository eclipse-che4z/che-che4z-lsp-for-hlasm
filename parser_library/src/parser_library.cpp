#include <iostream>

#include "../include/shared/parser_library.h"

namespace hlasm_plugin {
	namespace parser_library {

		//testing output
		void parser_library::parse(const std::string & src)
		{	
			antlr4::ANTLRInputStream input(src);
			lexer lexer(&input);
			antlr4::CommonTokenStream tokens(&lexer);
			generated::hlasmparser parser(&tokens);
			auto vocab = parser.getVocabulary();
			
			parser.lexer = &lexer;
			auto tree = parser.program();
			//std::ofstream tokenstream;
			//tokenstream.open("C:/Users/hruma02/Desktop/HlasmPlugin/parser_library/test/res/output/tokens/correctness.output");
			/*
			for (auto && token : tokens.getTokens())
			{
				//tokenstream << vocab.getSymbolicName(token->getType()) << std::endl;
				auto type = token->getType();
				std::cout << vocab.getSymbolicName(type);
				if (type != lexer.EOLLN && type != lexer.SPACE && type != lexer.CONTINUATION && type != lexer.IGNORED) std::cout << "---" << "\"" << token->getText() << "\"";
				std::cout << std::endl;
			}
			*/
			//tokenstream.close();

			hlasm_plugin::parser_tools::useful_tree mytree(tree, vocab, parser.getRuleNames());
			mytree.out_tree(std::cout);
		}
	}
}
