#include "lsp_info_processor.h"
#include "../context/instruction.h"
#include <string_view>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::context;

lsp_info_processor::lsp_info_processor(std::string file, const std::string& text, context::lsp_ctx_ptr ctx)
: file_name(std::move(file)), text_(text), ctx_(ctx), instruction_regex("^(\\S)*\\s+\\S*")
{
	hl_info_.document = { file_name };

	if (!ctx_->initialized)
	{
		for (const auto& machine_instr : instruction::machine_instructions)
		{
			std::stringstream ss(" ");
			bool first = true;
			for (const auto& op : machine_instr.operands)
			{
				if (!first)
					ss << ",";
				else
					first = false;
				ss << instruction::op_format_to_string.at(op);
			}
			ss << " [" << instruction::mach_format_to_string.at(machine_instr.format) << "]";
			ctx_->all_instructions.push_back({ machine_instr.name,1,ss.str(),"Machine",false,machine_instr.name + "   " + ss.str() } );
		}

		for (const auto& asm_instr : instruction::assembler_instructions)
		{
			std::stringstream ss(" ");
			bool first = true;
			for (int i = 0; i < asm_instr.max_operands; ++i)
			{
				if (!first)
					ss << ",";
				else
					first = false;
				if (i >= asm_instr.min_operands)
					ss << "?";
				ss << "OP" << i + 1;
			}
			deferred_instruction_.value = ss.str();
			ctx_->all_instructions.push_back({ asm_instr.name,1,ss.str(),"Assembler",false,asm_instr.name + "   " + ss.str() });
		}

		for (const auto& mnemonic_instr : instruction::mnemonic_codes)
		{
			std::stringstream ss(" ");
			ss << "Mnemonic code for " << mnemonic_instr.machine_instr << " with operand " << mnemonic_instr.operand;
			ctx_->all_instructions.push_back({ mnemonic_instr.extended_branch,1,"",ss.str(),false,mnemonic_instr.extended_branch });
		}

		for (const auto& ca_instr : instruction::ca_instructions)
		{
			ctx_->all_instructions.push_back({ ca_instr,1,"","Conditional Assembly",false,ca_instr });
		}

		for (const auto& macro_instr : instruction::macro_processing_instructions)
		{
			ctx_->all_instructions.push_back({ macro_instr,1,"","Macro Processing",false,macro_instr });
		}

		ctx_->initialized = true;
	}
};

void lsp_info_processor::process_hl_symbols(std::vector<token_info> symbols)
{
	for (const auto& symbol : symbols)
	{
		add_hl_symbol(symbol);
	}
}

void lsp_info_processor::process_lsp_symbols(std::vector<lsp_symbol> symbols)
{
	for (const auto & symbol : symbols)
	{
		add_lsp_symbol(symbol);
	}
	process_instruction_sym_();
	process_var_syms_();
}

bool lsp_info_processor::find_definition_(const position & pos,const definitions & symbols, position_uri_s & found) const
{
	for (auto it = symbols.begin(); it != symbols.end(); ++it)
	{
		if (is_in_range_(pos, it->second))
		{
			found = { it->first.file_name, {it->first.range.begin_ln, it->first.range.begin_col} };
			return true;
		}
	}
	return false;
}

bool lsp_info_processor::find_references_(const position & pos, const definitions & symbols, std::vector<position_uri_s>& found) const
{
	for (auto it = symbols.begin(); it != symbols.end(); ++it)
	{
		if (is_in_range_(pos, it->second))
		{
			auto range = symbols.equal_range(it->first);
			for (auto i = range.first; i != range.second; ++i)
				found.push_back({ i->second.file_name,{i->second.range.begin_ln,i->second.range.begin_col}});

			return true;
		}
	}
	return false;
}



completion_list_s lsp_info_processor::completion(const position& pos, const char trigger_char, int trigger_kind) const
{
	size_t start = 0;
	auto line = get_line_(pos.line+1,start);
	auto line_so_far = line.substr(0, (pos.column == 0) ? 1 : (const unsigned int)pos.column);
	char last_char = (trigger_kind == 1 && line_so_far != "") ? line_so_far.back() : trigger_char;

	if (last_char == '&')
		return complete_var_(pos);
	else if (last_char == '.')
		return complete_seq_(pos);
	else if (std::regex_match(line_so_far, instruction_regex))
		return { false, ctx_->all_instructions };

	return { false, {} };
}

std::string lsp_info_processor::get_line_(position_t line_no, size_t& start) const
{
	auto new_line = '\n';
	size_t end = text_.find(new_line, start);
	size_t line_count = 0;
	while (end != std::string::npos)
	{
		line_count++;
		if (line_count == line_no)
		{
			auto result = text_.substr(start, end - start);
			start = end + 1;
			return result;
		}
		start = end+1;
		end = text_.find(new_line, start);
	}
	// last line without new line at the end
	line_count++;
	if (line_count == line_no)
	{
		auto result = text_.substr(start);
		start = end + 1;
		return result;
	}
	return "";
};

position_uri_s lsp_info_processor::go_to_definition(const position & pos) const
{
	position_uri_s result;
	if (find_definition_(pos, ctx_->seq_symbols, result) || find_definition_(pos, ctx_->var_symbols, result) || find_definition_(pos, ctx_->ord_symbols, result) || find_definition_(pos, ctx_->instructions, result))
		return result;
	return { file_name,pos };
}
std::vector<position_uri_s> lsp_info_processor::references(const position & pos) const
{
	std::vector<position_uri_s> result;
	if (find_references_(pos, ctx_->seq_symbols, result) || find_references_(pos, ctx_->var_symbols, result) || find_references_(pos, ctx_->ord_symbols, result) || find_references_(pos, ctx_->instructions, result))
		return result;
	return { { file_name,pos } };
}
std::vector<std::string> lsp_info_processor::hover(const position & pos) const
{
	std::vector<std::string> result;
	if (get_text_(pos, ctx_->seq_symbols,result) || get_text_(pos, ctx_->var_symbols, result) || get_text_(pos, ctx_->ord_symbols, result) || get_text_(pos, ctx_->instructions, result))
		return result;
	return result;
}
void lsp_info_processor::add_lsp_symbol(lsp_symbol symbol)
{
	std::string scope;
	if (parser_macro_stack_.size() > 0)
		scope = parser_macro_stack_.top();
	definition symbol_def(symbol.name, scope, file_name, symbol.range);
	switch (symbol.type)
	{
	case symbol_type::ord:
		process_ord_sym_(symbol_def);
		break;
	case symbol_type::var:
		deferred_vars_.push_back(symbol_def);
		break;
	case symbol_type::instruction:
		deferred_instruction_ = {symbol.name,scope,file_name,symbol.range,false};
		break;
	case symbol_type::seq:
		process_seq_sym_(symbol_def);
		break;
	}
}

void hlasm_plugin::parser_library::semantics::lsp_info_processor::add_hl_symbol(token_info symbol)
{
	if (symbol.scope == hl_scopes::continuation)
	{
		hl_info_.cont_info.continuation_positions.push_back({ symbol.token_range.start.line, symbol.token_range.start.column });
	}
	hl_info_.lines.push_back(symbol);
}

semantics::highlighting_info & hlasm_plugin::parser_library::semantics::lsp_info_processor::get_hl_info()
{
	return hl_info_;
}

bool lsp_info_processor::is_in_range_(const position& pos, const occurence& occ) const
{
	//check for multi line
	if (occ.range.begin_ln != occ.range.end_ln)
	{
		if (file_name != occ.file_name)
			return false;
		if (pos.line < occ.range.begin_ln || pos.line > occ.range.end_ln)
			return false;
		// find appropriate line
		for (const auto& cont_pos : hl_info_.cont_info.continuation_positions)
		{
			// might be multi line
			if (cont_pos.line == pos.line)
			{
				// occurences begin line, position cannot be smaller than occ begin column or bigger than continuation column
				if (pos.line == occ.range.begin_ln && (pos.column < occ.range.begin_col || pos.column > cont_pos.column))
					return false;
				// occurences end line, position cannot be bigger than occ end column or smaller than continue column
				if (pos.line == occ.range.end_ln && (pos.column > occ.range.end_col || pos.column < hl_info_.cont_info.continue_column))
					return false;
				// in between begin and end lines, only check for continue/continuation columns
				if (pos.column < hl_info_.cont_info.continue_column || pos.column > cont_pos.column)
					return false;
				return true;
			}
		}
	}
	// no continuation, symbol is single line 
	return file_name == occ.file_name && pos.line == occ.range.begin_ln && pos.line == occ.range.end_ln && pos.column >= occ.range.begin_col && pos.column <= occ.range.end_col;
}

bool lsp_info_processor::get_text_(const position& pos, const definitions& symbols, std::vector<std::string>& found) const
{
	for (auto it = symbols.begin(); it != symbols.end(); ++it)
	{
		if (is_in_range_(pos, it->second))
		{
			found.push_back(it->first.name + " " + it->first.value);
			found.push_back(it->first.documentation);

			return true;
		}
	}
	return false;
}

void lsp_info_processor::process_ord_sym_(const definition & symbol)
{
	/* TODO ORD SYMS
	*/
}

void lsp_info_processor::process_var_syms_()
{
	for (auto & symbol : deferred_vars_)
	{
		std::stringstream ss;
		auto it = ctx_->var_symbols.find(symbol);
		//there is definition, add occurence to it
		if (it != ctx_->var_symbols.end())
			ctx_->var_symbols.insert({ it->first,{symbol.range,symbol.file_name} });
		//first occurence, create new definition
		else
		{
			if (deferred_instruction_.name == "SETC")
				symbol.value = "string";
			else if (deferred_instruction_.name == "SETB")
				symbol.value = "bool";
			else if (deferred_instruction_.name == "SETA")
				symbol.value = "number";
			//newly defined var symbol but not by set instructions, must be defined by macro
			else if (parser_macro_stack_.size() > 0)
				symbol.scope = parser_macro_stack_.top();
			else
				return;
			ctx_->var_symbols.insert({ symbol,{symbol.range,symbol.file_name} });
		}
	}
	deferred_vars_.clear();
	deferred_instruction_ = definition();
}
			
void lsp_info_processor::process_seq_sym_(definition & symbol)
{
	auto it = ctx_->seq_symbols.find(symbol);
	//there is definition, add occurence
	if (it != ctx_->seq_symbols.end())
	{
		ctx_->seq_symbols.insert({ it->first,{symbol.range,symbol.file_name} });
	}
	else
	{
		//is definition, create it
		if (symbol.range.begin_col == 0)
		{
			std::stringstream ss;
			ss << "Defined at line " << symbol.range.begin_ln + 1;
			symbol.value = ss.str();
			//add deferred if its matching current definition
			auto temp_seqs = deferred_seqs_;
			for (const auto & def_sym : deferred_seqs_)
			{
				//there is definition, add occurence to it and remove it from deferred
				if (def_sym == symbol)
				{
					std::remove(temp_seqs.begin(), temp_seqs.end(), def_sym);
					ctx_->seq_symbols.insert({ symbol,{def_sym.range,def_sym.file_name} });
				}
			}
			std::swap(deferred_seqs_, temp_seqs);
			ctx_->seq_symbols.insert({ symbol,{symbol.range,symbol.file_name} });
		}
		//not a definition, remember it
		else
			deferred_seqs_.push_back(symbol);
	}
}
void lsp_info_processor::process_instruction_sym_()
{
	if (deferred_instruction_.name == "MACRO")
	{
		parser_macro_stack_.push("");
		return;
	}
	else if (!parser_macro_stack_.empty() && deferred_instruction_.name == "MEND")
	{
		parser_macro_stack_.pop();
		return;
	}
	//define macro
	else if (!parser_macro_stack_.empty() && parser_macro_stack_.top() == "")
	{
		if (deferred_instruction_.name == "")
		{
			parser_macro_stack_.top() = "ASPACE";
			return;
		}

		std::stringstream ss;
		parser_macro_stack_.top() = deferred_instruction_.name;
		size_t index = 0;
		//before parameter
		if (!deferred_vars_.empty() && deferred_vars_[0].range.begin_col == 0)
		{
			ss << deferred_vars_[0].name << " ";
			deferred_vars_[0].scope = deferred_instruction_.name;
			index++;
		}
		//name
		bool first = true;
		//after parameters
		for (size_t i = index; i < deferred_vars_.size(); ++i)
		{
			if (!first)
				ss << ",";
			else
				first = false;
			ss << deferred_vars_[i].name;
			deferred_vars_[i].scope = deferred_instruction_.name;
		}
		deferred_instruction_.value = ss.str();
		std::stringstream out_ss("Macro");
		size_t start = 0;
		std::string line = get_line_(deferred_instruction_.range.begin_ln + 1, start);
		for (size_t i = 0; i < 10; i++)
		{
			line = get_line_(1, start);
			if (line.size() < 2 || line.front() != '*')
				break;
			std::string_view line_view(line);
			line_view.remove_prefix(1);
			out_ss << line_view << std::endl;
		}

		deferred_instruction_.documentation = out_ss.str();

		ctx_->all_instructions.push_back({ deferred_instruction_.name,1,deferred_instruction_.value,deferred_instruction_.documentation,false,deferred_instruction_.name + "   " + deferred_instruction_.value });
		ctx_->instructions.insert({ deferred_instruction_,{deferred_instruction_.range,deferred_instruction_.file_name} });
	}
	else
	{
		//check for already defined instruction
		auto it = ctx_->instructions.find(deferred_instruction_);
		if (it != ctx_->instructions.end())
			ctx_->instructions.insert({ it->first,{deferred_instruction_.range,deferred_instruction_.file_name} });
		//define new instruction
		else
		{
			auto instr = std::find_if(ctx_->all_instructions.begin(), ctx_->all_instructions.end(),
				[this](const context::completion_item_s & instr) { return instr.label == deferred_instruction_.name; });
			if (instr != ctx_->all_instructions.end())
			{
				deferred_instruction_.value = instr->detail;
				deferred_instruction_.documentation = instr->documentation;
				ctx_->instructions.insert({ deferred_instruction_,{deferred_instruction_.range,deferred_instruction_.file_name} });
			}
		}
	}
}

completion_list_s lsp_info_processor::complete_var_(const position& pos) const
{
	std::vector<context::completion_item_s> items;
	for (auto it = ctx_->var_symbols.begin(), end = ctx_->var_symbols.end();
		it != end;
		it = ctx_->var_symbols.upper_bound(it->first)
		)
	{
		if (it->first.range.end_ln < pos.line && it->first.file_name == file_name)
			items.push_back({ "&" + it->first.name,1,it->first.value,"",false,it->first.name });
	}
	return { false,items };
}

completion_list_s lsp_info_processor::complete_seq_(const position& pos) const
{
	std::vector<context::completion_item_s> items;
	for (auto it = ctx_->seq_symbols.begin(), end = ctx_->seq_symbols.end();
		it != end;
		it = ctx_->seq_symbols.upper_bound(it->first)
		)
	{
		if (it->first.range.end_ln < pos.line && it->first.file_name == file_name)
			items.push_back({ "." + it->first.name,1,it->first.value,"",false,it->first.name });
	}
	return { false, items };
}
