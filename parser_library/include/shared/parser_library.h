#ifndef HLASMPLUGIN_PARSER_LIBRARY_H
#define HLASMPLUGIN_PARSER_LIBRARY_H

#include <string>

#include "../generated/parser_library_export.h"
#include "lexer.h"
#include "antlr4-runtime.h"
#include "../generated/hlasmparser.h"

namespace hlasm_plugin {
	namespace parser_library {

		class parser_library
		{
		public:
			parser_library() {};
			void PARSER_LIBRARY_EXPORT parse(const std::string &);
		};
	} //namespace parser_library
} //namespace hlasm_plugin


#endif
