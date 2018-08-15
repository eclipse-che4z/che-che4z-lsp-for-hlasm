#ifndef HLASMPLUGIN_PARSER_LIBRARY_H
#define HLASMPLUGIN_PARSER_LIBRARY_H

#include <string>

#include "../generated/parser_library_export.h"
#include "lexer.h"
#include "antlr4-runtime.h"
#include "../generated/hlasmparser.h"

namespace HlasmPlugin {
	namespace HlasmParserLibrary {

		class UsefulTree
		{
		public:
			UsefulTree(antlr4::ParserRuleContext * _tree, HlasmGenerated::hlasmparser & parser);
			void outTree(std::ostream &stream);
		private:
			antlr4::ParserRuleContext * tree;
			antlr4::dfa::Vocabulary &vocab;
			std::vector<std::string> rules;
			antlr4::TokenStream *tokens;

			void outTreeRec(antlr4::ParserRuleContext * tree, std::string indent, std::ostream & stream);
		};

		class HlasmParserLibrary
		{
		public:
			HlasmParserLibrary() {};
			void PARSER_LIBRARY_EXPORT parse(const std::string &);
		};
	} //namespace HlasmParserLibrary
} //namespace HlasmPlugin


#endif
