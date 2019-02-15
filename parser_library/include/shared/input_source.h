#ifndef HLASMPLUGIN_PARSER_HLASMINPUTSOURCE_H
#define HLASMPLUGIN_PARSER_HLASMINPUTSOURCE_H

#include "parser_library_export.h"
#include "antlr4-runtime.h"

namespace hlasm_plugin {
	namespace parser_library {
		class PARSER_LIBRARY_EXPORT input_source : public antlr4::ANTLRInputStream
		{
		public:
			input_source(const std::string & input);

			void append(const UTF32String &str);
			void rewind_input(size_t index);

			input_source(const input_source &) = delete;
			input_source& operator=(const input_source&) = delete;
			input_source& operator=(input_source&&) = delete;
			input_source(input_source &&) = delete;

			virtual std::string getText(const antlr4::misc::Interval &interval) override;

			virtual ~input_source() = default;
		};
	}
}

#endif
