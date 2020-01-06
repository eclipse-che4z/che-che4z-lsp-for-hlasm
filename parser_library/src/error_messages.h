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

#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_MESSAGES_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_MESSAGES_H

#include "diagnostic.h"

namespace hlasm_plugin {
	namespace parser_library {

		class error_messages
		{
		public:
			using diag_ptr = std::unique_ptr< diagnostic_op>;

			static inline diag_ptr not_implemented()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "IMPL", "Not implemented");
			}

			static inline diag_ptr e001() 
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "E001", "Invalid expression");
			}

			static inline diag_ptr e002()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "E002", "Invalid expression: a maximum of 24 operators can be used");
			}

			static inline diag_ptr e003()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "E003", "Invalid self-defining term");
			}

			static inline diag_ptr e004()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "E004", "Use of undefined ordinary symbol identifier");
			}

			static inline diag_ptr e005()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "E005", "Only absolute and defined symbols allowed");
			}

			static inline diag_ptr ec01()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC01", "Duplication cannot be negative");
			}

			static inline diag_ptr ec02()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC02", "Invalid substring expressions");
			}

			static inline diag_ptr ec03()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error,
						"EC03",
						"Character expression expected");
			}

			static inline diag_ptr ec04()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error,
						"EC04",
						"Character expression cannot be empty");
			}

			static inline diag_ptr ec05()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC05", "Undefined operation");
			}

			static inline diag_ptr ec06()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC06", "Invalid binary number");
			}

			static inline diag_ptr ec07()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC07", "Invalid decimal number");
			}

			static inline diag_ptr ec08()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC08", "Number out of range");
			}

			static inline diag_ptr ec09()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC09", "Invalid hexadecimal number");
			}

			static inline diag_ptr ea01()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA01", "Number out of range");
			}

			static inline diag_ptr ea02(const std::string & number)
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA02", "Invalid number " + number);
			}

			static inline diag_ptr ea03()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA03", "Unrecognized self-defining term");
			}

			static inline diag_ptr ea04()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA04", "Argument too long (a maximum of 4 characters allowed)");
			}

			static inline diag_ptr ea05()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA05", "DBCS is not enabled, graphic expression is not allowed");
			}

			static inline diag_ptr ea06()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA06", "ASMA148E Self-defining term lacks ending quote or has bad character");
			}

			static inline diag_ptr ea07()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA07", "Invalid expression type, can be logic or arithmetic");
			}

			static inline diag_ptr ea08()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA08", "Unsupported arithmetic operation");
			}

			static inline diag_ptr ea09()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA09", "Invalid expression (arithmetic expected)");
			}

			static inline diag_ptr ea10()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA10", "Arithmetic operation overflow");
			}

			static inline diag_ptr ea11()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA11", "Argument out of range (must be 0 - 255)");
			}

			static inline diag_ptr el01()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EL01", "Invalid expression type, can be logic or arithmetic");
			}

			static inline diag_ptr el02()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EL02", "Undefined operation");
			}
		};

	}
}

#endif
