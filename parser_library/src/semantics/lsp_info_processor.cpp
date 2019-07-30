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
			for (size_t i = 0; i < machine_instr.second->operands.size(); i++)
			{
				const auto& op = machine_instr.second->operands[i];
				if (machine_instr.second->no_optional == 1 && machine_instr.second->operands.size() - i == 1)
				{
					ss << "[";
					if (i != 0)
						ss << ",";
					ss << op.to_string() << "]";
				}
				else if (machine_instr.second->no_optional == 2 && machine_instr.second->operands.size() - i == 2)
				{
					ss << "[";
					if (i != 0)
						ss << ",";
					ss << op.to_string() << "[,";
				}
				else if (machine_instr.second->no_optional == 2 && machine_instr.second->operands.size() - i == 1)
				{
					ss << op.to_string() << "]]";
				}
				else
				{
					if (i != 0)
						ss << ",";
					ss << op.to_string();
				}
		
			}
			ss << " [" << instruction::mach_format_to_string.at(machine_instr.second->format) << "]";
			ctx_->all_instructions.push_back({ machine_instr.first,1,ss.str(),"Machine",false, machine_instr.first + "   " + ss.str() } );
		}

		for (const auto& asm_instr : instruction::assembler_instructions)
		{
			int min_op = asm_instr.second.min_operands;
			int max_op = asm_instr.second.max_operands;
			std::stringstream ss(" ");
			bool first = true;
			for (int i = 0; i < max_op; ++i)
			{
				if (!first)
					ss << ",";
				else
					first = false;
				if (i >= min_op)
					ss << "?";
				ss << "OP" << i + 1;
			}
			deferred_instruction_.value = ss.str();
			ctx_->all_instructions.push_back({ asm_instr.first,1,ss.str(),"Assembler",false,asm_instr.first + "   " + ss.str() });
		}

		for (const auto& mnemonic_instr : instruction::mnemonic_codes)
		{
			std::stringstream ss(" ");
			std::stringstream subs_ops_mnems (" ");
			std::stringstream subs_ops_nomnems(" ");

			// get mnemonic operands
			size_t iter_over_mnem = 0;

			auto instr_name = mnemonic_instr.second.instruction;
			auto mach_operands = instruction::machine_instructions[instr_name]->operands;
			auto no_optional = instruction::machine_instructions[instr_name]->no_optional;
			bool first = true;

			auto replaces = mnemonic_instr.second.replaced;

			for (size_t i = 0; i < mach_operands.size(); i++)
			{
				if (replaces.size() > iter_over_mnem)
				{
					auto [position, value] = replaces[iter_over_mnem];
					// can still replace mnemonics
					if (position == i)
					{
						// mnemonics can be substituted when no_optional is 1, but not 2 -> 2 not implemented
						if (no_optional == 1 && mach_operands.size() - i == 1)
						{
							subs_ops_mnems << "[";
							if (i != 0)
								subs_ops_mnems << ",";
							subs_ops_mnems << std::to_string(value) + "]";
							continue;
						}
						// replace current for mnemonic
						if (i != 0)
							subs_ops_mnems << ",";
						subs_ops_mnems << std::to_string(value);
						iter_over_mnem++;
						continue;
					}
				}
				// do not replace by a mnemonic
				std::string curr_op_with_mnem = "";
				std::string curr_op_without_mnem = "";
				if (no_optional == 0)
				{
					if (i != 0)
						curr_op_with_mnem += ",";
					if (!first)
						curr_op_without_mnem += ",";
					curr_op_with_mnem += mach_operands[i].to_string();
					curr_op_without_mnem += mach_operands[i].to_string();
				}
				else if (no_optional == 1 && mach_operands.size() - i == 1)
				{
					curr_op_with_mnem += "[";
					curr_op_without_mnem += "[";
					if (i != 0)
						curr_op_with_mnem += ",";
					if (!first)
						curr_op_without_mnem += ",";
					curr_op_with_mnem += mach_operands[i].to_string() + "]";
					curr_op_without_mnem += mach_operands[i].to_string() + "]";
				}
				else if (no_optional == 2 && mach_operands.size() - i == 1)
				{
					curr_op_with_mnem += mach_operands[i].to_string() + "]]";
					curr_op_without_mnem += mach_operands[i].to_string() + "]]";
				}
				else if (no_optional == 2 && mach_operands.size() - i == 2)
				{
					curr_op_with_mnem += "[";
					curr_op_without_mnem += "[";
					if (i != 0)
						curr_op_with_mnem += ",";
					if (!first)
						curr_op_without_mnem += ",";
					curr_op_with_mnem += mach_operands[i].to_string() + "[,";
					curr_op_without_mnem += mach_operands[i].to_string() + "[,";
				}
				subs_ops_mnems << curr_op_with_mnem;
				subs_ops_nomnems << curr_op_without_mnem;
				first = false;
			}
			subs_ops_mnems << " [" << instruction::mach_format_to_string.at(instruction::machine_instructions[instr_name]->format) << "]";
			ss << "Mnemonic code for " << instr_name << " with operands " << subs_ops_mnems.str();
			ctx_->all_instructions.push_back({ mnemonic_instr.first,1,"",ss.str(),false, mnemonic_instr.first + "   " + subs_ops_nomnems.str() });
		}

		for (const auto& ca_instr : instruction::ca_instructions)
		{
			ctx_->all_instructions.push_back({ ca_instr.name,1,"","Conditional Assembly",false,ca_instr.name });
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
			found = { it->first.file_name, it->first.definition_range.start };
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
				found.push_back({ i->second.file_name,i->second.definition_range.start });

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
	definition symbol_def(symbol.name, scope, file_name, symbol.symbol_range);
	switch (symbol.type)
	{
	case symbol_type::ord:
		process_ord_sym_(symbol_def);
		break;
	case symbol_type::var:
		deferred_vars_.push_back(symbol_def);
		break;
	case symbol_type::instruction:
		deferred_instruction_ = {symbol.name,scope,file_name,symbol.symbol_range,false};
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
	if (occ.definition_range.start.line != occ.definition_range.end.line)
	{
		if (file_name != occ.file_name)
			return false;
		if (pos.line < occ.definition_range.start.line || pos.line > occ.definition_range.end.line)
			return false;
		// find appropriate line
		for (const auto& cont_pos : hl_info_.cont_info.continuation_positions)
		{
			// might be multi line
			if (cont_pos.line == pos.line)
			{
				// occurences begin line, position cannot be smaller than occ begin column or bigger than continuation column
				if (pos.line == occ.definition_range.start.line && (pos.column < occ.definition_range.start.column || pos.column > cont_pos.column))
					return false;
				// occurences end line, position cannot be bigger than occ end column or smaller than continue column
				if (pos.line == occ.definition_range.end.line && (pos.column > occ.definition_range.end.column || pos.column < hl_info_.cont_info.continue_column))
					return false;
				// in between begin and end lines, only check for continue/continuation columns
				if (pos.column < hl_info_.cont_info.continue_column || pos.column > cont_pos.column)
					return false;
				return true;
			}
		}
	}
	// no continuation, symbol is single line 
	return file_name == occ.file_name && pos.line == occ.definition_range.start.line && pos.line == occ.definition_range.end.line && pos.column >= occ.definition_range.start.column && pos.column <= occ.definition_range.end.column;
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
			ctx_->var_symbols.insert({ it->first,{symbol.definition_range,symbol.file_name} });
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
			ctx_->var_symbols.insert({ symbol,{symbol.definition_range,symbol.file_name} });
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
		ctx_->seq_symbols.insert({ it->first,{symbol.definition_range,symbol.file_name} });
	}
	else
	{
		//is definition, create it
		if (symbol.definition_range.start.column == 0)
		{
			std::stringstream ss;
			ss << "Defined at line " << symbol.definition_range.start.line + 1;
			symbol.value = ss.str();
			//add deferred if its matching current definition
			decltype(deferred_seqs_) temp_seqs;
			for (auto& def_sym : deferred_seqs_)
			{
				//there is definition, add occurence to it and remove it from deferred
				if (def_sym == symbol)
				{
					ctx_->seq_symbols.insert({ symbol,{def_sym.definition_range,def_sym.file_name} });
				}
				else
				{
					temp_seqs.push_back(std::move(def_sym));
				}
			}
			deferred_seqs_ = std::move(temp_seqs);
			ctx_->seq_symbols.insert({ symbol,{symbol.definition_range,symbol.file_name} });
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
		if (!deferred_vars_.empty() && deferred_vars_[0].definition_range.start.column == 0)
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
		std::string line = get_line_(deferred_instruction_.definition_range.start.line + 1, start);
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
		ctx_->instructions.insert({ deferred_instruction_,{deferred_instruction_.definition_range,deferred_instruction_.file_name} });
	}
	else
	{
		//check for already defined instruction
		auto it = ctx_->instructions.find(deferred_instruction_);
		if (it != ctx_->instructions.end())
			ctx_->instructions.insert({ it->first,{deferred_instruction_.definition_range,deferred_instruction_.file_name} });
		//define new instruction
		else
		{
			auto instr = std::find_if(ctx_->all_instructions.begin(), ctx_->all_instructions.end(),
				[this](const context::completion_item_s & instr) { return instr.label == deferred_instruction_.name; });
			if (instr != ctx_->all_instructions.end())
			{
				deferred_instruction_.value = instr->detail;
				deferred_instruction_.documentation = instr->documentation;
				ctx_->instructions.insert({ deferred_instruction_,{deferred_instruction_.definition_range,deferred_instruction_.file_name} });
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
		if (it->first.definition_range.end.line < pos.line && it->first.file_name == file_name)
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
		if (it->first.definition_range.end.line < pos.line && it->first.file_name == file_name)
			items.push_back({ "." + it->first.name,1,it->first.value,"",false,it->first.name });
	}
	return { false, items };
}
