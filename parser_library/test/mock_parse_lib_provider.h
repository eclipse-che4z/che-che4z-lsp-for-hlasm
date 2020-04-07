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

constexpr const char* MACRO_FILE = "MAC";
constexpr const char* SOURCE_FILE = "OPEN";
constexpr const char* COPY_FILE = "path/COPYFILE";

namespace hlasm_plugin::parser_library {

class mock_parse_lib_provider : public parse_lib_provider
{
public:
	virtual parse_result parse_library(const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data) override
	{
		(void)library;

		if (data.proc_kind == processing::processing_kind::MACRO)
		{
			analyzer a(macro_contents, MACRO_FILE, hlasm_ctx, *this, data);
			a.analyze();
		}
		else
		{
			analyzer a(copy_contents, COPY_FILE, hlasm_ctx, *this, data);
			a.analyze();
		}
		return true;
	}
	virtual bool has_library(const std::string&, context::hlasm_context&) const override { return true; }
private:
	const std::string macro_contents =
		R"(   MACRO
       MAC   &VAR
       LR    &VAR,&VAR
       MEND
)";
	const std::string copy_contents =
		R"(R2 EQU 2
			LR R2,R2)";
};

}