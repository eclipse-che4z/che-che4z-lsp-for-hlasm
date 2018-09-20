#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_MESSAGES_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_MESSAGES_H

#include "diagnosable.h"

namespace hlasm_plugin {
	namespace parser_library {

		class error_messages
		{
		public:
			using diag_ptr = std::unique_ptr< diagnostic_op>;

			static inline diag_ptr not_implemented()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "IMPL", "not implemented");
			}

			static inline diag_ptr e001() 
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "E001", "invalid expression");
			}

			static inline diag_ptr e002()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "E002", "invalid expression: max 24 operators can be used");
			}

			static inline diag_ptr e003()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "E005", "invalid self-defining termm");
			}

			static inline diag_ptr ec01()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC01", "duplication cannot be negative");
			}

			static inline diag_ptr ec02()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC02", "invalid substring expressions");
			}

			static inline diag_ptr ec03()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error,
						"EC03",
						"character expression expected");
			}

			static inline diag_ptr ec04()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error,
						"EC04",
						"character expression cannot be empty");
			}

			static inline diag_ptr ec05()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC05", "undefined operation");
			}

			static inline diag_ptr ec06()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC06", "invalid binary number");
			}

			static inline diag_ptr ec07()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC07", "invalid decimal number");
			}

			static inline diag_ptr ec08()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC08", "number out of range");
			}

			static inline diag_ptr ec09()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EC09", "invalid hexadecimal number");
			}

			static inline diag_ptr ea01()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA01", "number out of range");
			}

			static inline diag_ptr ea02()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA02", "invalid number");
			}

			static inline diag_ptr ea03()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA03", "unrecognized self-defining term");
			}

			static inline diag_ptr ea04()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA04", "too long argument (max 4 characters)");
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
					(diagnostic_severity::error, "EA07", "invalid expression type, can be logic or arithmetic");
			}

			static inline diag_ptr ea08()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA08", "not supported arithmetic operation");
			}

			static inline diag_ptr ea09()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA09", "invalid expression (arithmetic expected)");
			}

			static inline diag_ptr ea10()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA10", "arithmetic operation overflow");
			}

			static inline diag_ptr ea11()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EA11", "argument out of range (must be 0 - 255)");
			}

			static inline diag_ptr el01()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EL01", "invalid expression type, can be logic or arithmetic");
			}

			static inline diag_ptr el02()
			{
				return std::make_unique<diagnostic_op>
					(diagnostic_severity::error, "EL02", "undefined operation");
			}
		};

	}
}

#endif
