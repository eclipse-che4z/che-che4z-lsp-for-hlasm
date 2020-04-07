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

#ifndef HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_CTX_H
#define HLASMPLUGIN_PARSERLIBRARY_ERROR_LISTENER_CTX_H

#include "parser_error_listener.h"
#include "context/hlasm_context.h"
#include "semantics/range_provider.h"

//implementation of parser error listener that provide additional error handling
//used during recursed parsing when nested diagnostic is needed
namespace hlasm_plugin::parser_library::parsing {

class parser_error_listener_ctx : public parser_error_listener_base, public diagnosable_ctx
{
public:
	parser_error_listener_ctx
	(context::hlasm_context& hlasm_ctx, std::optional<std::string> substituted, semantics::range_provider provider = semantics::range_provider());

	virtual void collect_diags() const override;

protected:
	virtual void add_parser_diagnostic(range diagnostic_range, diagnostic_severity severity, std::string code, std::string message) override;

private:
	std::optional<std::string> substituted_;
	semantics::range_provider provider_;
};

}

#endif
