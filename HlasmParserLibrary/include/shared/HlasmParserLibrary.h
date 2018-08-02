#ifndef HLASMPLUGIN_PARSER_LIBRARY_H
#define HLASMPLUGIN_PARSER_LIBRARY_H

#include <string>

#include "HlasmParserLibrary_export.h"


namespace HlasmPlugin {
namespace HlasmParserLibrary {

class HlasmParserLibrary
	{
	public:
		void static HLASMPARSERLIBRARY_EXPORT parse(std::string &&);
	};
} //namespace HlasmParserLibrary
} //namespace HlasmPlugin


#endif