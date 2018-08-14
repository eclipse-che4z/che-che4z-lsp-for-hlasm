#include <iostream>

#include "../include/shared/HlasmParserLibrary.h"


namespace HlasmPlugin {
	namespace HlasmParserLibrary {

		void HlasmParserLibrary::parse(const std::string & src)
		{
			antlr4::ANTLRInputStream input(src);
			HlasmLexer lexer(&input);
			antlr4::CommonTokenStream tokens(&lexer);
			tokens.fill();
			HlasmGenerated::HlasmParser parser(&tokens);
			auto vocab = parser.getVocabulary();
			for (auto && token : tokens.getTokens())
			{
				auto type = token->getType();
				std::cout << vocab.getSymbolicName(type);
				if (type != lexer.EOLLN && type != lexer.SPACE && type != lexer.IGNORED && type != lexer.CONTINUATION) std::cout << "---" << "\"" << token->getText() << "\"";
				std::cout << std::endl;
			}

			parser.lexer = &lexer;
			auto tree = parser.program();
			UsefulTree mytree(tree, parser);
			mytree.outTree(std::cout);
		}


		UsefulTree::UsefulTree(antlr4::ParserRuleContext * _tree, HlasmGenerated::HlasmParser & parser)
			: vocab(parser.getVocabulary()), tree(_tree), rules(parser.getRuleNames()), tokens(parser.getTokenStream())
		{};

		void UsefulTree::outTree(std::ostream &stream)
		{
			outTreeRec(tree, "", stream);
		}

		void UsefulTree::outTreeRec(antlr4::ParserRuleContext * tree, std::string indent, std::ostream & stream)
		{
			if (tree->children.empty())
			{
				if (tree->getText() == "")
					stream << indent << rules[tree->getRuleIndex()] << ": " << "\"" << tree->getText() << "\"" << std::endl;
				else
				{
					auto type = tokens->get(tree->getSourceInterval().a)->getType();
					stream << indent << vocab.getSymbolicName(type);
					if (type != HlasmLexer::EOLLN && type != HlasmLexer::SPACE) stream << ": " << "\"" << tree->getText() << "\"";
					stream << std::endl;
				}
			}
			else
			{
				stream << indent << rules[tree->getRuleIndex()] << ": " << "\"" << tree->getText() << "\"" << std::endl;
				indent.insert(0, "\t");
				for (auto child : tree->children)
				{
					outTreeRec((antlr4::ParserRuleContext *)child, indent, stream);
				}
			}
		}
	};
}
