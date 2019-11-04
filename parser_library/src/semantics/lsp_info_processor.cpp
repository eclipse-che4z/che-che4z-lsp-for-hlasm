#include "lsp_info_processor.h"
#include "../context/instruction.h"
#include <string_view>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::parser_library::context;

lsp_info_processor::lsp_info_processor(std::string file, const std::string& text, context::lsp_ctx_ptr ctx)
: file_name(std::move(file)), ctx_(ctx), instruction_regex("^([^*][^*]\\S*\\s+\\S+|\\s+\\S*)")
{
	// initialize text vector
	text_ = split_(text);

	hl_info_.document = { file_name };

	// initialize context
	if (!ctx_->initialized)
	{
		for (const auto& machine_instr : instruction::machine_instructions)
		{
			std::stringstream documentation(" ");
			std::stringstream detail(""); // operands used for hover - e.g. V,D12U(X,B)[,M]
			std::stringstream autocomplete(""); // operands used for autocomplete - e.g. V,D12U(X,B) [,M]
			for (size_t i = 0; i < machine_instr.second->operands.size(); i++)
			{
				const auto& op = machine_instr.second->operands[i];
				if (machine_instr.second->no_optional == 1 && machine_instr.second->operands.size() - i == 1)
				{
					autocomplete << " [";
					detail << "[";
					if (i != 0)
					{
						autocomplete << ",";
						detail << ",";
					}
					detail << op.to_string() << "]";
					autocomplete << op.to_string() << "]";
				}
				else if (machine_instr.second->no_optional == 2 && machine_instr.second->operands.size() - i == 2)
				{
					autocomplete << " [";
					detail << "[";
					if (i != 0)
					{
						autocomplete << ",";
						detail << ",";
					}
					detail << op.to_string() << "]";
					autocomplete << op.to_string() << "[,";
				}
				else if (machine_instr.second->no_optional == 2 && machine_instr.second->operands.size() - i == 1)
				{
					detail << op.to_string() << "]]";
					autocomplete << op.to_string() << "]]";
				}
				else
				{
					if (i != 0)
					{
						autocomplete << ",";
						detail << ",";
					}
					detail << op.to_string();
					autocomplete << op.to_string();
				}
			}
			documentation << "Machine instruction " << std::endl << "Instruction format: " << instruction::mach_format_to_string.at(machine_instr.second->format);
			ctx_->all_instructions.push_back({ machine_instr.first,1,"Operands: " + detail.str(),documentation.str(),false, machine_instr.first + "   " + autocomplete.str() } );
		}

		for (const auto& asm_instr : instruction::assembler_instructions)
		{
			std::stringstream documentation(" ");
			std::stringstream detail("");

			//int min_op = asm_instr.second.min_operands;
			//int max_op = asm_instr.second.max_operands;
			std::string description = asm_instr.second.description;
			
			deferred_instruction_.value.push_back(description);
			detail << asm_instr.first << "   " << description;
			documentation << "Assembler instruction";
			ctx_->all_instructions.push_back({ asm_instr.first,1,detail.str(),documentation.str(),false,asm_instr.first + "   " /*+ description*/ });
		}

		for (const auto& mnemonic_instr : instruction::mnemonic_codes)
		{
			std::stringstream documentation(" ");
			std::stringstream detail("");
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
			detail << "Operands: " + subs_ops_nomnems.str();
			documentation << "Mnemonic code for " << instr_name << " instruction" << std::endl << "Substituted operands: " << subs_ops_mnems.str() << std::endl << "Instruction format: " << 
				instruction::mach_format_to_string.at(instruction::machine_instructions[instr_name]->format);
			ctx_->all_instructions.push_back({ mnemonic_instr.first,1,detail.str(),documentation.str(),false, mnemonic_instr.first + "   " + subs_ops_nomnems.str() });
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

void lsp_info_processor::process_lsp_symbols(std::vector<lsp_symbol> symbols,const std::string & given_file)
{
	bool only_ord = false;
	auto symbol_file = file_name;
	// if the file is given, process only ordinary symbols
	if (given_file != "")
	{
		only_ord = true;
		symbol_file = given_file;
	}
	// the order of the symbols cannot change
	for (auto& symbol : symbols)
	{
		symbol.symbol_range.file = symbol_file;
		if (!only_ord || symbol.type == symbol_type::ord)
			add_lsp_symbol(symbol);
	}
	if (!only_ord)
	{
		process_instruction_sym_();
		process_var_syms_();
	}
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
	std::string line_before = (pos.line > 0) ? text_[(const unsigned int)pos.line-1] : "";
	auto line = text_[(const unsigned int)pos.line];
	auto line_so_far = line.substr(0, (pos.column == 0) ? 1 : (const unsigned int)pos.column);
	char last_char = (trigger_kind == 1 && line_so_far != "") ? line_so_far.back() : trigger_char;

	if (last_char == '&')
		return complete_var_(pos);
	else if (last_char == '.')
		return complete_seq_(pos);
	else if (std::regex_match(line_so_far, instruction_regex) && (line_before.size() <= hl_info_.cont_info.continuation_column || std::isspace(line_before[hl_info_.cont_info.continuation_column])))
		return { false, ctx_->all_instructions };

	return { false, {} };
}

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
	if (ctx_->parser_macro_stack.size() > 0)
		scope = ctx_->parser_macro_stack.top();
	definition symbol_def(symbol.name, scope, symbol.symbol_range.file, symbol.symbol_range.r, { symbol.value });
	switch (symbol.type)
	{
	case symbol_type::ord:
		process_ord_sym_(symbol_def);
		break;
	case symbol_type::var:
		deferred_vars_.push_back(symbol_def);
		break;
	case symbol_type::instruction:
		deferred_instruction_ = { symbol.name,scope,symbol.symbol_range.file,symbol.symbol_range.r,{""},false };
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
			found = it->first.value;
			return true;
		}
	}
	return false;
}

void lsp_info_processor::process_ord_sym_(context::definition & symbol)
{
	if (deferred_instruction_.name == "COPY")
	{
		ctx_->deferred_ord_occs.push_back(symbol);
		ctx_->copy = true;
		return;
	}
	if (ctx_->parser_macro_stack.empty())
		symbol.check_scopes = false;
	// to be processed after parsing
	if (symbol.definition_range.start.column == 0)
		ctx_->deferred_ord_defs.push_back(symbol);
	else
		ctx_->deferred_ord_occs.push_back(symbol);

	/*
	// definition
	if (symbol.definition_range.start.column == 0)
	{
		ctx_->ord_symbols.insert({ symbol, { symbol.definition_range,symbol.file_name } });
		// check for deferred
		for (const auto& deferred_sym : ctx_->deferred_ords)
			if (deferred_sym == symbol)
				ctx_->ord_symbols.insert({ symbol, { deferred_sym.definition_range,deferred_sym.file_name } });
		ctx_->deferred_ords.erase(std::remove(ctx_->deferred_ords.begin(), ctx_->deferred_ords.end(), symbol), ctx_->deferred_ords.end());
	}
	else
	{
		// occurence
		auto it = ctx_->ord_symbols.find(symbol);
		if (it != ctx_->ord_symbols.end())
			ctx_->ord_symbols.insert({ it->first, { symbol.definition_range,symbol.file_name } });
		else
			ctx_->deferred_ords.push_back(symbol);
	}*/
}

void lsp_info_processor::process_var_syms_()
{
	for (auto & symbol : deferred_vars_)
	{
		// no scopes in copy/open code
		if (ctx_->parser_macro_stack.empty())
			symbol.check_scopes = false;
		
		// latest version of the symbol
		auto latest = find_latest_version_(symbol,ctx_->var_symbols);
		//redefinition
		if (latest.name != "")
			symbol.version = latest.version + 1;
		std::stringstream version_string;
		version_string << "version " << symbol.version;

		definition * def = &symbol;
		// check if it is a definition
		if (deferred_instruction_.name == "SETC" && symbol.definition_range.start.column == 0)
			symbol.value = { "string", version_string.str() };
		else if (deferred_instruction_.name == "SETB" && symbol.definition_range.start.column == 0)
			symbol.value = { "bool", version_string.str() };
		else if (deferred_instruction_.name == "SETA" && symbol.definition_range.start.column == 0)
			symbol.value = { "number", version_string.str() };
		else if (deferred_instruction_.name == "GBLC" && symbol.definition_range.start.column != 0)
		{
			symbol.value = { "global string", version_string.str() };
			symbol.check_scopes = false;
		}
		else if (deferred_instruction_.name == "GBLB" && symbol.definition_range.start.column != 0)
		{
			symbol.value = { "global bool", version_string.str() };
			symbol.check_scopes = false;
		}
		else if (deferred_instruction_.name == "GBLA" && symbol.definition_range.start.column != 0)
		{
			symbol.value = { "global number", version_string.str() };
			symbol.check_scopes = false;
		}
		else if (deferred_instruction_.name == "LCLC" && symbol.definition_range.start.column != 0)
			symbol.value = { "local string", version_string.str() };
		else if (deferred_instruction_.name == "LCLB" && symbol.definition_range.start.column != 0)
			symbol.value = { "local bool", version_string.str() };
		else if (deferred_instruction_.name == "LCLA" && symbol.definition_range.start.column != 0)
			symbol.value = { "local number", version_string.str() };
		// macro params
		else if (!ctx_->parser_macro_stack.empty() && ctx_->parser_macro_stack.top() == deferred_instruction_.name)
		{
			symbol.scope = deferred_instruction_.name;
			symbol.value = { "Macro Param" };
		}
		// not a definition, only occurence
		else
			def = &latest;

		//add to the symbols
		ctx_->var_symbols.insert({ *def, {symbol.definition_range,symbol.file_name} });
	}
	//clean
	deferred_vars_.clear();
	deferred_instruction_ = definition();
}
			
void lsp_info_processor::process_seq_sym_(definition & symbol)
{
	if (ctx_->parser_macro_stack.empty())
		symbol.check_scopes = false;
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
			symbol.value = { "Defined at line " + std::to_string(symbol.definition_range.start.line + 1) };
			//add deferred if its matching current definition
			decltype(ctx_->deferred_seqs) temp_seqs;
			for (auto& def_sym : ctx_->deferred_seqs)
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
			ctx_->deferred_seqs = std::move(temp_seqs);
			ctx_->seq_symbols.insert({ symbol,{symbol.definition_range,symbol.file_name} });
		}
		//not a definition, remember it
		else
			ctx_->deferred_seqs.push_back(symbol);
	}
}
void lsp_info_processor::process_instruction_sym_()
{
	if (ctx_->copy && deferred_instruction_.name != "COPY")
	{
		auto deferred_ord = ctx_->deferred_ord_occs.back();
		auto copy_def = deferred_instruction_;
		copy_def.value= {"Defined in file: " + copy_def.file_name};
		ctx_->ord_symbols.insert({ copy_def,{deferred_ord.definition_range,deferred_ord.file_name} });
		ctx_->deferred_ord_occs.pop_back();
		ctx_->copy = false;
	}

	if (deferred_instruction_.name == "MACRO")
	{
		ctx_->parser_macro_stack.push("");
		return;
	}
	else if (!ctx_->parser_macro_stack.empty() && deferred_instruction_.name == "MEND")
	{
		ctx_->parser_macro_stack.pop();
		return;
	}
	//define macro
	else if (!ctx_->parser_macro_stack.empty() && ctx_->parser_macro_stack.top() == "")
	{
		if (deferred_instruction_.name == "")
		{
			ctx_->parser_macro_stack.top() = "ASPACE";
			return;
		}

		//find if the macro already exists
		// create new version of it
		auto latest = find_latest_version_(deferred_instruction_, ctx_->instructions);
		if (latest.name != "")
			deferred_instruction_.version = latest.version + 1;

		std::stringstream ss;
		ctx_->parser_macro_stack.top() = deferred_instruction_.name;
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
		ss << " (version " << deferred_instruction_.version << ")";
		deferred_instruction_.value = { ss.str() };

		for (size_t i = 1; i <= 10; i++)
		{
			if (text_.size() <= (const unsigned int)deferred_instruction_.definition_range.start.line + i)
				break;
			std::string_view line = text_[(const unsigned int)deferred_instruction_.definition_range.start.line + i];
			if (line.size() < 2 || line.front() != '*')
				break;
			line.remove_prefix(1);
			deferred_instruction_.value.push_back(line.data());
		}

		assert(!deferred_instruction_.value.empty());
		ctx_->all_instructions.push_back({ deferred_instruction_.name,1,deferred_instruction_.value[0],implode_(deferred_instruction_.value.begin() +1, deferred_instruction_.value.end()),false,deferred_instruction_.name + "   " + deferred_instruction_.value[0] });
		ctx_->instructions.insert({ deferred_instruction_,{deferred_instruction_.definition_range,deferred_instruction_.file_name} });

		if (ctx_->deferred_macro_statement.name == deferred_instruction_.name)
		{
			ctx_->instructions.insert({ deferred_instruction_,{ctx_->deferred_macro_statement.definition_range,ctx_->deferred_macro_statement.file_name} });
			ctx_->deferred_macro_statement = definition();
		}
	}
	else
	{
		//check for already defined instruction
		// need to find the latest version
		auto latest = find_latest_version_(deferred_instruction_,ctx_->instructions);
		if (latest.name != "")
			ctx_->instructions.insert({ latest,{deferred_instruction_.definition_range,deferred_instruction_.file_name} });
		//define new instruction
		else
		{
			auto instr = std::find_if(ctx_->all_instructions.begin(), ctx_->all_instructions.end(),
				[this](const context::completion_item_s & instr) { return instr.label == deferred_instruction_.name; });
			if (instr != ctx_->all_instructions.end())
			{
				deferred_instruction_.value = { instr->detail };
				auto split_doc = split_(instr->documentation);
				deferred_instruction_.value.insert(deferred_instruction_.value.end(),split_doc.begin(),split_doc.end());
				ctx_->instructions.insert({ deferred_instruction_,{deferred_instruction_.definition_range,deferred_instruction_.file_name} });
			}
			// undefined instruction, special case when macro name statement goes before macro parsing
			// or an error
			else
			{
				ctx_->deferred_macro_statement = deferred_instruction_;
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
		{
			assert(!it->first.value.empty());
			items.push_back({ "&" + it->first.name,1,it->first.value[0],implode_(it->first.value.begin() + 1, it->first.value.end()),false,it->first.name });
		}
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
			items.push_back({ "." + it->first.name,1,it->first.value[0],"",false,it->first.name });
	}
	return { false, items };
}

context::definition hlasm_plugin::parser_library::semantics::lsp_info_processor::find_latest_version_(const context::definition& current, const context::definitions& to_check) const
{
	auto redefinition = current;
	redefinition.version = std::numeric_limits<decltype(redefinition.version)>::max();
	auto candidate = to_check.upper_bound(redefinition);
	if (candidate == to_check.begin())
		return definition();
	--candidate;
	if (candidate->first.name != current.name)
		return definition();
	return candidate->first;
}

std::vector<std::string> hlasm_plugin::parser_library::semantics::lsp_info_processor::split_(const std::string& text)
{
	std::vector<std::string> result;
	std::stringstream ss(text);
	std::string line;
	while (std::getline(ss, line))
		result.push_back(line);
	return result;
}

std::string hlasm_plugin::parser_library::semantics::lsp_info_processor::implode_(const std::vector<std::string>::const_iterator& lines_begin, const std::vector<std::string>::const_iterator& lines_end) const
{
	std::stringstream result;
	for (auto it = lines_begin; it != lines_end; ++it)
		result << *it << '\n';
	return result.str();
}
