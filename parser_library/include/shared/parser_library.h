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
