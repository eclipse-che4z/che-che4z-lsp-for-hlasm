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

#ifndef PROCESSING_MACRO_PROCESSOR_H
#define PROCESSING_MACRO_PROCESSOR_H

#include "instruction_processor.h"
#include "context/macro.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

struct macro_arguments 
{
	context::macro_data_ptr name_param;
	std::vector<context::macro_arg> symbolic_params;
};

//processor of macro instructions
class macro_processor : public instruction_processor
{
public:
	macro_processor(context::hlasm_context& hlasm_ctx,
		attribute_provider& attr_provider, branching_provider& branch_provider, workspaces::parse_lib_provider& lib_provider);
	virtual void process(context::unique_stmt_ptr stmt) override;
	virtual void process(context::shared_stmt_ptr stmt) override;

	static context::macro_data_ptr string_to_macrodata(std::string data);

private:
	macro_arguments get_args(const resolved_statement& statement) const;
};

}
}
}
#endif