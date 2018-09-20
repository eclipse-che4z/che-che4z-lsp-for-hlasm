#ifndef HLASMPLUGIN_PARSER_HLASMEBCDIC_H
#define HLASMPLUGIN_PARSER_HLASMEBCDIC_H

#include  <string>

namespace hlasm_plugin {
	namespace parser_library {
		namespace context {
			class ebcdic_encoding
			{
			public:
				static unsigned char a2e[256];

				static unsigned char e2a[256];

				static unsigned char to_pseudoascii(const char*& c);

				static unsigned char to_ebcdic(unsigned char c);

				static std::string to_ebcdic(const std::string& s);

				static std::string to_ascii(unsigned char c);

				static std::string to_ascii(const std::string& s);
			};
		}
	}
}

#endif
