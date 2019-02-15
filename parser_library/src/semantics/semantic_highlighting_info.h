#ifndef SEMANTIC_HIGHLIGHTING_INFO
#define SEMANTIC_HIGHLIGHTING_INFO

#include <map>
#include <string>
#include <vector>

#include "shared/protocol.h"
#include "operand.h"

namespace hlasm_plugin {
namespace parser_library {
namespace semantics {

struct versioned_document
{ 
	versioned_document() {};
	versioned_document(std::string uri) : uri(std::move(uri)) {}
	std::string uri;
	version_t version;
};

using lines_info = std::vector<token_info>;

struct semantic_highlighting_info
{
	semantic_highlighting_info() {};
	semantic_highlighting_info(std::string document_uri) : document(std::move(document_uri)) {}

	semantic_highlighting_info(versioned_document document, lines_info lines) : document(std::move(document)), lines(std::move(lines)) {};
	versioned_document document;
	lines_info lines;
};

}
}
}

#endif
