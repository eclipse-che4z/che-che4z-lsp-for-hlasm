/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#ifndef HLASMPLUGIN_PARSER_HLASMINPUTSOURCE_H
#define HLASMPLUGIN_PARSER_HLASMINPUTSOURCE_H

#include "parser_library_export.h"
#include "antlr4-runtime.h"

namespace hlasm_plugin {
	namespace parser_library {
		/*
			custom ANTLRInputStream
			supports input rewinding, appending and resetting
		*/
		class PARSER_LIBRARY_EXPORT input_source : public antlr4::ANTLRInputStream
		{
		public:
			input_source(const std::string & input);

			void append(const UTF32String &str);
			void append(const std::string &str);
			using antlr4::ANTLRInputStream::reset;
			void reset(const std::string &str);
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
