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

#include "debugger.h"
#include "../analyzer.h"
#include "macro_param_variable.h"
#include "set_symbol_variable.h"
#include "ordinary_symbol_variable.h"

namespace hlasm_plugin::parser_library::debugging
{

breakpoints debug_config::get_breakpoints(const std::string& source)
{
	std::lock_guard guard(bpoints_mutex_);
	auto res = bpoints_.emplace(source, breakpoints(source, {}));
	return res.first->second;
}

void debug_config::set_breakpoints(breakpoints breakpoints)
{
	std::lock_guard guard(bpoints_mutex_);
	auto res = bpoints_.emplace(breakpoints.bps_source.path, breakpoints);
	if (!res.second)
		res.first->second = breakpoints;
}

debugger::debugger(debug_event_consumer_s & event_consumer, debug_config & debug_cfg) :
	event_(event_consumer), cfg_(debug_cfg)
{
}

void debugger::launch(processor_file_ptr open_code, parse_lib_provider & provider, bool stop_on_entry)
{
	opencode_source_path_ = open_code->get_file_name();
	auto b = std::bind(&debugger::debug_start, this, std::placeholders::_1, std::placeholders::_2);
	stop_on_next_stmt_ = stop_on_entry;
	
	thread_ = std::make_unique<std::thread>(b, open_code, &provider);
}

void debugger::statement(range stmt_range)
{
	if (disconnected_)
		return;

	bool breakpoint_hit = false;
	auto bps = cfg_.get_breakpoints(ctx_->processing_stack().back().proc_location.file);
	for (auto & bp : bps.points)
	{
		if (bp.line >= stmt_range.start.line && bp.line <= stmt_range.end.line)
			breakpoint_hit = true;
	}
	

	//breakpoint check
	if (stop_on_next_stmt_ ||
		breakpoint_hit ||
		(step_over_ && ctx_->processing_stack().size() <= step_over_depth_))
	{
		variables_.clear();
		stack_frames_.clear();
		scopes_.clear();
		proc_stack_ = ctx_->processing_stack();
		variable_mtx_.unlock();
		
		std::unique_lock<std::mutex> lck(control_mtx);
		if (disconnected_)
			return;
		stop_on_next_stmt_ = false;
		step_over_ = false;
		next_stmt_range_ = stmt_range;

		continue_ = false;
		event_.stopped("entry", "");


		con_var.wait(lck, [&]{ return !!continue_; });
		
		variable_mtx_.lock();

	}
	
}

void debugger::next()
{
	{
		std::lock_guard<std::mutex> lck(control_mtx);
		step_over_ = true;
		step_over_depth_ = ctx_->processing_stack().size();
		continue_ = true;
	}
	con_var.notify_all();
}

void debugger::step_in()
{
	{
		std::lock_guard<std::mutex> lck(control_mtx);
		stop_on_next_stmt_ = true;
		continue_ = true;
	}
	con_var.notify_all();
}

void debugger::disconnect()
{
	//set cancelation token and wake up the thread,
	//so it peacefully finishes, we then wait for it and join
	{
		std::lock_guard<std::mutex> lck(control_mtx);

		disconnected_ = true;
		continue_ = true;
		cancel_ = true;
	}
	con_var.notify_all();
	
	thread_->join();
}

void debugger::continue_debug()
{
	stop_on_next_stmt_ = false;
	continue_ = true;
	con_var.notify_all();
}

const std::vector<stack_frame> & debugger::stack_frames()
{
	std::lock_guard<std::mutex> guard(variable_mtx_);
	stack_frames_.clear();
	
	for(size_t i = proc_stack_.size()-1; i != -1; --i)
	{
		source source(proc_stack_[i].proc_location.file);
		std::string name;
		switch (proc_stack_[i].proc_type)
		{
		case context::file_processing_type::OPENCODE:
			name = "OPENCODE";
			break;
		case context::file_processing_type::MACRO:
			name = "MACRO";
			break;
		case context::file_processing_type::COPY:
			name = "COPY";
			break;
		default:
			name = "";
			break;
		}

		stack_frames_.emplace_back(proc_stack_[i].proc_location.pos.line, proc_stack_[i].proc_location.pos.line, (uint32_t) i, std::move(name), std::move(source));
	}


	return stack_frames_;
}

const std::vector<scope> & debugger::scopes(frame_id_t frame_id)
{
	std::lock_guard<std::mutex> guard(variable_mtx_);
	scopes_.clear();

	if (frame_id >= proc_stack_.size())
		return scopes_;

	std::vector<variable_ptr> scope_vars;
	std::vector<variable_ptr> globals;
	std::vector<variable_ptr> ordinary_symbols;
	//we show only global variables that are valid for current scope,
	//moreover if we show variable in globals, we do not show it in locals
	

	

	if(proc_stack_[frame_id].scope.is_in_macro())
		for (auto it : proc_stack_[frame_id].scope.this_macro->named_params)
		{
			if (it.first == context::id_storage::empty_id)
				continue;
			scope_vars.push_back(std::make_unique<macro_param_variable>(*it.second, std::vector<size_t>{}));
		}

	for (auto it : proc_stack_[frame_id].scope.variables)
	{
		if(it.second->is_global)
			globals.push_back(std::make_unique<set_symbol_variable>(*it.second));
		else
			scope_vars.push_back(std::make_unique<set_symbol_variable>(*it.second));
	}

	for (auto it : proc_stack_[frame_id].scope.system_variables)
	{
		if (it.second->is_global)
			globals.push_back(std::make_unique<macro_param_variable>(*it.second, std::vector<size_t>{}));
		else
			scope_vars.push_back(std::make_unique<macro_param_variable>(*it.second, std::vector<size_t>{}));
		//fetch all vars
	}

	for (const auto & it : ctx_->ord_ctx.get_all_symbols())
		ordinary_symbols.push_back(std::make_unique<ordinary_symbol_variable>(it.second));
	


	variables_[next_var_ref_] = std::move(globals);
	scopes_.emplace_back("Globals", next_var_ref_++, source{ opencode_source_path_ });

	variables_[next_var_ref_] = std::move(scope_vars);
	scopes_.emplace_back("Locals", next_var_ref_++, source{opencode_source_path_});

	variables_[next_var_ref_] = std::move(ordinary_symbols);
	scopes_.emplace_back("Ordinary symbols", next_var_ref_++, source{ opencode_source_path_ });
	return scopes_;
}


std::vector<variable_ptr> empty_variables;

const std::vector<variable_ptr> & debugger::variables(var_reference_t var_ref)
{
	std::lock_guard<std::mutex> guard(variable_mtx_);
	auto it = variables_.find(var_ref);
	if (it == variables_.end())
		return empty_variables;
	for (const variable_ptr & var : variables_[var_ref])
	{
		if (var->is_scalar())
			continue;

		variables_.emplace(next_var_ref_, var->values() );
		var->var_reference = next_var_ref_;
		++next_var_ref_;
	}

	return variables_[var_ref];
}

debugger::~debugger()
{
	if(thread_->joinable())
		thread_->join();
}




void debugger::debug_start(processor_file_ptr open_code, parse_lib_provider * provider)
{
	std::lock_guard<std::mutex> guard(variable_mtx_);
	analyzer a(open_code->get_text(), open_code->get_file_name(), *provider, this);
	
	ctx_ = &a.context();
	
	a.analyze(&cancel_);

	if (!disconnected_)
		event_.exited(0);
}

}