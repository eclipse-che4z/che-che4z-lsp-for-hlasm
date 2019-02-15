#ifndef HLASMPLUGIN_PARSER_LIBRARY_H
#define HLASMPLUGIN_PARSER_LIBRARY_H

#include <string>

#include "parser_library_export.h"
#include "lexer.h"
#include "antlr4-runtime.h"

namespace hlasm_plugin {
	namespace parser_library {

		class PARSER_LIBRARY_EXPORT parser_library
		{
		public:
			parser_library() {};
			void parse(const std::string &);
		};
	} //namespace parser_library
} //namespace hlasm_plugin


#endif
