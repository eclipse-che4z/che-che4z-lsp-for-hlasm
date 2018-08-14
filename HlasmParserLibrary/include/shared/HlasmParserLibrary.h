#ifndef HLASMPLUGIN_PARSER_LIBRARY_H
#define HLASMPLUGIN_PARSER_LIBRARY_H

#include <string>

#include "HlasmParserLibrary_export.h"
#include "HlasmLexer.h"
#include "antlr4-runtime.h"
#include "../generated/HlasmParser.h"

namespace HlasmPlugin {
	namespace HlasmParserLibrary {

		class UsefulTree
		{
		public:
			UsefulTree(antlr4::ParserRuleContext * _tree, HlasmGenerated::HlasmParser & parser);
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
			void HLASMPARSERLIBRARY_EXPORT parse(const std::string &);
		};
	} //namespace HlasmParserLibrary
} //namespace HlasmPlugin


#endif
