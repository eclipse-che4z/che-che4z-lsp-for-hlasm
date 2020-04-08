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

#include "analyzer.h"
#include "parsing/error_strategy.h"
#include "processing/processing_tracer.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lexing;
using namespace hlasm_plugin::parser_library::parsing;
using namespace hlasm_plugin::parser_library::workspaces;

analyzer::analyzer(
	const std::string& text,
	std::string file_name,
	parse_lib_provider& lib_provider,
	context::hlasm_context* hlasm_ctx,
	const library_data data,
	bool own_ctx,
	processing::processing_tracer* tracer,
	bool collect_hl_info)
	: diagnosable_ctx(*hlasm_ctx),
	hlasm_ctx_(own_ctx ? hlasm_ctx : nullptr), hlasm_ctx_ref_(*hlasm_ctx),
	listener_(file_name),
	lsp_proc_(file_name, text, hlasm_ctx, collect_hl_info),
	input_(text), lexer_(&input_, &lsp_proc_, &hlasm_ctx_ref_.metrics), tokens_(&lexer_), parser_(new parsing::hlasmparser(&tokens_)),
	mngr_(std::unique_ptr<processing::opencode_provider>(parser_), hlasm_ctx_ref_, data, file_name, lib_provider, *parser_, tracer)
{
	parser_->initialize(&hlasm_ctx_ref_, &lsp_proc_);
	parser_->setErrorHandler(std::make_shared<error_strategy>());
	parser_->removeErrorListeners();
	parser_->addErrorListener(&listener_);
}

analyzer::analyzer(const std::string& text, std::string file_name, context::hlasm_context& hlasm_ctx, parse_lib_provider& lib_provider, const library_data data, bool collect_hl_info)
	: analyzer(text, file_name, lib_provider, &hlasm_ctx, data, false, nullptr,collect_hl_info) {}

analyzer::analyzer(const std::string& text, std::string file_name, parse_lib_provider& lib_provider, processing::processing_tracer* tracer, bool collect_hl_info)
	: analyzer(
		text,
		file_name,
		lib_provider,
		new context::hlasm_context(file_name),
		library_data{ processing::processing_kind::ORDINARY,context::id_storage::empty_id },
		true,
		tracer,
		collect_hl_info) {}

context::hlasm_context& analyzer::context()
{
	return hlasm_ctx_ref_;
}

parsing::hlasmparser& analyzer::parser()
{
	return *parser_;
}

semantics::lsp_info_processor& analyzer::lsp_processor()
{
	return lsp_proc_;
}

void analyzer::analyze(std::atomic<bool>* cancel)
{
	mngr_.start_processing(cancel);
}

void analyzer::collect_diags() const
{
	collect_diags_from_child(mngr_);
	collect_diags_from_child(listener_);
}

const performance_metrics& analyzer::get_metrics()
{
	hlasm_ctx_ref_.fill_metrics_files();
	return hlasm_ctx_ref_.metrics;
}

