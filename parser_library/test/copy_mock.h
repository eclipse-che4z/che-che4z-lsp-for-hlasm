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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_COPY_MOCK_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_COPY_MOCK_H

#include "analyzer.h"

namespace hlasm_plugin::parser_library {

class copy_mock : public parse_lib_provider
{
	const std::string* find_content(const std::string& library) const
	{
		if (library == "COPYR")
			return &content_COPYR;
		else if (library == "COPYF")
			return &content_COPYF;
		else if (library == "COPYD")
			return &content_COPYD;
		else if (library == "COPYREC")
			return &content_COPYREC;
		else if (library == "COPYU")
			return &content_COPYU;
		else if (library == "COPYL")
			return &content_COPYL;
		else if (library == "COPYN")
			return &content_COPYN;
		else if (library == "MAC")
			return &content_MAC;
		else if (library == "COPYM")
			return &content_COPYM;
		else if (library == "COPYJ")
			return &content_COPYJ;
		else if (library == "COPYJF")
			return &content_COPYJF;
		else if (library == "COPYND1")
			return &content_COPYND1;
		else if (library == "COPYND2")
			return &content_COPYND2;
		else if (library == "COPYBM")
			return &content_COPYBM;
		else
			return nullptr;
	}

public:


	virtual parse_result parse_library(const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data)
	{
		current_content = find_content(library);
		if (!current_content) return false;

		holder.push_back(std::move(a));
		a = std::make_unique<analyzer>(*current_content, library, hlasm_ctx, *this, data);
		a->analyze();
		a->collect_diags();
		return true;
	}
	virtual bool has_library(const std::string& library, context::hlasm_context& hlasm_ctx) const
	{
		(void)hlasm_ctx;
		return find_content(library);
	}
	std::vector<std::unique_ptr<analyzer>> holder;
	std::unique_ptr<analyzer> a;
private:
	const std::string* current_content;

	const std::string content_COPYR =
		R"(   
 LR 1,1
 MACRO
 M1
 LR 1,1
 
 MACRO
 M2
 LR 2,2
 MEND
 AGO .A
.A ANOP
 MEND

&VARX SETA &VARX+1
.A ANOP
.B ANOP
&VAR SETA &VAR+1
)";
	const std::string content_COPYF =
		R"(  
 LR 1,1
&VARX SETA &VARX+1
 COPY COPYR
&VAR SETA &VAR+1
.C ANOP
)";

	const std::string content_COPYD =
		R"(  

 LR 1,
)";

	const std::string content_COPYREC =
		R"(  
 ANOP
 COPY COPYREC
 ANOP
)";

	const std::string content_COPYU =
		R"(  
 ANOP
 MACRO
 M
 MEND
 MEND
 ANOP
)";

	const std::string content_COPYL =
		R"(  
 LR 1,1
.A ANOP
&VARX SETA &VARX+1
 AGO .X
&VAR SETA &VAR+1
.A ANOP
.C ANOP
)";

	const std::string content_COPYN =
		R"( 
 MAC
)";

	const std::string content_MAC =
		R"( MACRO
 MAC
 LR 1,1
 COPY COPYM
 MEND
)";

	const std::string content_COPYM =
		R"(
.A ANOP
 GBLA &X
&X SETA 4
)";

	const std::string content_COPYJ =
		R"(
 AGO .X
 ;%
.X ANOP
)";
	const std::string content_COPYJF =
		R"(
 AGO .X
 LR
)";

	const std::string content_COPYND1 =
		R"(
 COPY COPYND2
)";

	const std::string content_COPYND2 =
		R"(



 LR 1,)";

	const std::string content_COPYBM =
		R"( 
 MACRO
 M
 LR 1
 MEND
)";
};

}

#endif
