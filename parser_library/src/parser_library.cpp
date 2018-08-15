#include <iostream>

#include "../include/shared/parser_library.h"


namespace hlasm_plugin {
	namespace parser_library {

		void parser_library::parse(const std::string & src)
		{
			antlr4::ANTLRInputStream input(src);
			HlasmLexer lexer(&input);
			antlr4::CommonTokenStream tokens(&lexer);
			tokens.fill();
			generated::hlasmparser parser(&tokens);
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
			useful_tree mytree(tree, parser);
			mytree.out_tree(std::cout);
		}


		useful_tree::useful_tree(antlr4::ParserRuleContext * _tree, generated::hlasmparser & parser)
			: vocab_(parser.getVocabulary()), tree_(_tree), rules_(parser.getRuleNames()), tokens_(parser.getTokenStream())
		{};

		void useful_tree::out_tree(std::ostream &stream)
		{
			out_tree_rec(tree_, "", stream);
		}

		void useful_tree::out_tree_rec(antlr4::ParserRuleContext * tree, std::string indent, std::ostream & stream)
		{
			if (tree->children.empty())
			{
				if (tree->getText() == "")
					stream << indent << rules_[tree->getRuleIndex()] << ": " << "\"" << tree->getText() << "\"" << std::endl;
				else
				{
					auto type = tokens_->get(tree->getSourceInterval().a)->getType();
					stream << indent << vocab_.getSymbolicName(type);
					if (type != HlasmLexer::EOLLN && type != HlasmLexer::SPACE) stream << ": " << "\"" << tree->getText() << "\"";
					stream << std::endl;
				}
			}
			else
			{
				stream << indent << rules_[tree->getRuleIndex()] << ": " << "\"" << tree->getText() << "\"" << std::endl;
				indent.insert(0, "\t");
				for (auto child : tree->children)
				{
					out_tree_rec((antlr4::ParserRuleContext *)child, indent, stream);
				}
			}
		}
	};
}
