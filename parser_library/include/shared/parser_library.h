#ifndef HLASMPLUGIN_PARSER_LIBRARY_H
#define HLASMPLUGIN_PARSER_LIBRARY_H

#include <string>

#include "../generated/parser_library_export.h"
#include "lexer.h"
#include "antlr4-runtime.h"
#include "../generated/hlasmparser.h"

namespace hlasm_plugin {
	namespace parser_library {

		class useful_tree
		{
		public:
			useful_tree(antlr4::ParserRuleContext * _tree, generated::hlasmparser & parser);
			void out_tree(std::ostream &stream);
		private:
			antlr4::ParserRuleContext * tree_;
			antlr4::dfa::Vocabulary &vocab_;
			std::vector<std::string> rules_;
			antlr4::TokenStream *tokens_;

			void out_tree_rec(antlr4::ParserRuleContext * tree, std::string indent, std::ostream & stream);
		};

		class parser_library
		{
		public:
			parser_library() {};
			void PARSER_LIBRARY_EXPORT parse(const std::string &);
		};
	} //namespace parser_library
} //namespace hlasm_plugin


#endif
