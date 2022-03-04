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

#include "hlasm_context.h"

#include <ctime>
#include <numeric>
#include <stdexcept>

#include "ebcdic_encoding.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_symbol_attribute.h"
#include "instruction.h"
#include "lexing/lexer.h"
#include "using.h"

namespace hlasm_plugin::parser_library::context {

code_scope* hlasm_context::curr_scope() { return &scope_stack_.back(); }

const code_scope* hlasm_context::curr_scope() const { return &scope_stack_.back(); }

hlasm_context::instruction_storage hlasm_context::init_instruction_map(id_storage& ids)
{
    hlasm_context::instruction_storage instr_map;
    for (const auto& instr : instruction::all_machine_instructions())
    {
        auto id = ids.add(std::string(instr.name()));
        instr_map.emplace(id, &instr);
    }
    for (const auto& instr : instruction::all_assembler_instructions())
    {
        auto id = ids.add(std::string(instr.name()));
        instr_map.emplace(id, &instr);
    }
    for (const auto& instr : instruction::all_ca_instructions())
    {
        auto id = ids.add(std::string(instr.name()));
        instr_map.emplace(id, &instr);
    }
    for (const auto& instr : instruction::all_mnemonic_codes())
    {
        auto id = ids.add(std::string(instr.name()));
        instr_map.emplace(id, &instr);
    }
    return instr_map;
}

namespace {
macro_data_ptr create_macro_data(std::string value)
{
    return std::make_unique<macro_param_data_single>(std::move(value));
}

macro_data_ptr create_macro_data(std::vector<std::string> value)
{
    std::vector<macro_data_ptr> data;
    for (auto& single_data : value)
    {
        data.push_back(std::make_unique<macro_param_data_single>(std::move(single_data)));
    }

    return std::make_unique<macro_param_data_composite>(std::move(data));
}

template<typename SYSTEM_VARIABLE_TYPE, typename DATA>
std::pair<id_index, sys_sym_ptr> create_system_variable(
    id_storage& ids, std::string name, DATA mac_data, bool is_global)
{
    auto id = ids.add(std::move(name));

    auto var = std::make_shared<SYSTEM_VARIABLE_TYPE>(id, create_macro_data(std::move(mac_data)), is_global);

    return { id, std::move(var) };
}

} // namespace

void hlasm_context::add_system_vars_to_scope(code_scope& scope)
{
    if (scope.is_in_macro())
    {
        {
            auto sect_name = ord_ctx.current_section() ? ord_ctx.current_section()->name : id_storage::empty_id;

            scope.system_variables.insert(
                create_system_variable<system_variable, std::string>(ids(), "SYSECT", *sect_name, false));
        }

        {
            std::string value = std::to_string(SYSNDX_);
            if (auto value_len = value.size(); value_len < 4)
                value.insert(0, 4 - value_len, '0');

            scope.system_variables.insert(
                create_system_variable<system_variable>(ids(), "SYSNDX", std::move(value), false));
        }

        {
            std::string value = "";

            if (ord_ctx.current_section())
            {
                switch (ord_ctx.current_section()->kind)
                {
                    case context::section_kind::COMMON:
                        value = "COM";
                        break;
                    case context::section_kind::DUMMY:
                        value = "DSECT";
                        break;
                    case context::section_kind::READONLY:
                        value = "RSECT";
                        break;
                    case context::section_kind::EXECUTABLE:
                        value = "CSECT";
                        break;
                    default:
                        break;
                }
            }

            scope.system_variables.insert(
                create_system_variable<system_variable>(ids(), "SYSSTYP", std::move(value), false));
        }

        {
            std::string location_counter_name = "";

            if (ord_ctx.current_section())
            {
                location_counter_name = *ord_ctx.current_section()->current_location_counter().name;
            }

            scope.system_variables.insert(
                create_system_variable<system_variable>(ids(), "SYSLOC", std::move(location_counter_name), false));
        }

        {
            std::string value = std::to_string(scope_stack_.size() - 1);

            scope.system_variables.insert(
                create_system_variable<system_variable>(ids(), "SYSNEST", std::move(value), false));
        }

        {
            std::vector<std::string> data;

            for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it)
            {
                std::string tmp;
                if (it->is_in_macro())
                    tmp = *it->this_macro->id;
                else
                    tmp = "OPEN CODE";
                data.push_back(std::move(tmp));
            }

            scope.system_variables.insert(
                create_system_variable<system_variable_sysmac>(ids(), "SYSMAC", std::move(data), false));
        }
    }
}

void hlasm_context::add_global_system_var_to_scope(id_storage& ids, std::string name, code_scope& scope) const
{
    auto id = ids.add(name);

    auto glob = globals_.find(id);

    sys_sym_ptr temp = std::dynamic_pointer_cast<system_variable>(glob->second);

    scope.system_variables.try_emplace(glob->second->id, temp);
}

void hlasm_context::add_global_system_vars(code_scope& scope)
{
    if (!is_in_macro())
    {
        {
            auto tmp_now = std::time(0);
            auto now = std::localtime(&tmp_now);

            std::string datc_val;
            std::string date_val;
            datc_val.reserve(8);
            date_val.reserve(8);
            auto year = std::to_string(now->tm_year + 1900);
            datc_val.append(year);

            if (now->tm_mon + 1 < 10)
            {
                datc_val.push_back('0');
                date_val.push_back('0');
            }

            datc_val.append(std::to_string(now->tm_mon + 1));

            date_val.append(std::to_string(now->tm_mon + 1));
            date_val.push_back('/');

            if (now->tm_mday < 10)
            {
                datc_val.push_back('0');
                date_val.push_back('0');
            }

            datc_val.append(std::to_string(now->tm_mday));

            date_val.append(std::to_string(now->tm_mday));
            date_val.push_back('/');
            date_val.append(year.c_str() + 2);

            {
                globals_.insert(create_system_variable<system_variable>(ids(), "SYSDATC", std::move(datc_val), true));
            }

            {
                globals_.insert(create_system_variable<system_variable>(ids(), "SYSDATE", std::move(date_val), true));
            }

            {
                std::string value;
                if (now->tm_hour < 10)
                    value.push_back('0');
                value.append(std::to_string(now->tm_hour));
                value.push_back(':');
                if (now->tm_min < 10)
                    value.push_back('0');
                value.append(std::to_string(now->tm_min));

                globals_.insert(create_system_variable<system_variable>(ids(), "SYSTIME", std::move(value), true));
            }
        }

        {
            globals_.insert(create_system_variable<system_variable>(ids(), "SYSPARM", asm_options_.sysparm, true));
        }

        {
            globals_.insert(create_system_variable<system_variable>(
                ids(), "SYSOPT_RENT", std::to_string(asm_options_.sysopt_rent), true));
        }

        {
            globals_.insert(create_system_variable<system_variable>(ids(), "SYSTEM_ID", asm_options_.system_id, true));
        }
    }

    add_global_system_var_to_scope(ids(), "SYSDATC", scope);
    add_global_system_var_to_scope(ids(), "SYSDATE", scope);
    add_global_system_var_to_scope(ids(), "SYSTIME", scope);
    add_global_system_var_to_scope(ids(), "SYSPARM", scope);
    add_global_system_var_to_scope(ids(), "SYSOPT_RENT", scope);
    add_global_system_var_to_scope(ids(), "SYSTEM_ID", scope);
}

bool hlasm_context::is_opcode(id_index symbol) const
{
    return macros_.find(symbol) != macros_.end() || instruction_map_.find(symbol) != instruction_map_.end();
}

hlasm_context::hlasm_context(std::string file_name, asm_option asm_options, std::shared_ptr<id_storage> init_ids)
    : ids_(std::move(init_ids))
    , opencode_file_name_(file_name)
    , asm_options_(std::move(asm_options))
    , instruction_map_(init_instruction_map(*ids_))
    , m_usings(std::make_unique<using_collection>())
    , m_active_usings(1, m_usings->remove_all())
    , ord_ctx(*ids_, *this)
{
    add_global_system_vars(scope_stack_.emplace_back());
    visited_files_.insert(file_name);
    push_statement_processing(processing::processing_kind::ORDINARY, std::move(file_name));
}

hlasm_context::~hlasm_context() = default;

void hlasm_context::set_source_position(position pos) { source_stack_.back().current_instruction.pos = pos; }

void hlasm_context::set_source_indices(size_t begin_index, size_t end_index, size_t end_line)
{
    source_stack_.back().begin_index = begin_index;
    source_stack_.back().end_index = end_index;
    source_stack_.back().end_line = end_line;
}

std::pair<source_position, source_snapshot> hlasm_context::get_begin_snapshot(bool ignore_macros) const
{
    context::source_position statement_position;

    bool is_in_macros = ignore_macros ? false : scope_stack_.size() > 1;

    if (!is_in_macros && current_copy_stack().empty())
    {
        statement_position.file_offset = current_source().begin_index;
        statement_position.file_line = current_source().current_instruction.pos.line;
    }
    else
    {
        statement_position.file_offset = current_source().end_index;
        statement_position.file_line = current_source().end_line + 1;
    }

    context::source_snapshot snapshot = current_source().create_snapshot();

    if (snapshot.copy_frames.size() && is_in_macros)
        ++snapshot.copy_frames.back().statement_offset;

    return std::make_pair(std::move(statement_position), std::move(snapshot));
}

std::pair<source_position, source_snapshot> hlasm_context::get_end_snapshot() const
{
    context::source_position statement_position;
    statement_position.file_offset = current_source().end_index;
    statement_position.file_line = current_source().end_line + 1;

    context::source_snapshot snapshot = current_source().create_snapshot();

    if (snapshot.copy_frames.size())
        ++snapshot.copy_frames.back().statement_offset;

    return std::make_pair(std::move(statement_position), std::move(snapshot));
}

void hlasm_context::push_statement_processing(const processing::processing_kind kind)
{
    assert(!source_stack_.empty());

    source_stack_.back().proc_stack.emplace_back(kind);
}

void hlasm_context::push_statement_processing(const processing::processing_kind kind, std::string file_name)
{
    source_stack_.emplace_back(std::move(file_name), kind);
}

void hlasm_context::pop_statement_processing()
{
    assert(!source_stack_.empty());
    assert(!source_stack_.back().proc_stack.empty());

    source_stack_.back().proc_stack.pop_back();

    if (source_stack_.back().proc_stack.empty())
        source_stack_.pop_back();
}

id_storage& hlasm_context::ids() { return *ids_; }

std::shared_ptr<id_storage> hlasm_context::ids_ptr() { return ids_; }

const hlasm_context::instruction_storage& hlasm_context::instruction_map() const { return instruction_map_; }

processing_stack_t hlasm_context::processing_stack() const
{
    std::vector<processing_frame> res;

    for (size_t i = 0; i < source_stack_.size(); ++i)
    {
        res.emplace_back(source_stack_[i].current_instruction,
            scope_stack_.front(),
            file_processing_type::OPENCODE,
            id_storage::empty_id);
        for (const auto& member : source_stack_[i].copy_stack)
        {
            location loc(member.current_statement_position(), member.definition_location()->file);
            res.emplace_back(std::move(loc), scope_stack_.front(), file_processing_type::COPY, member.name());
        }

        if (i == 0) // append macros immediately after ordinary processing
        {
            for (size_t j = 1; j < scope_stack_.size(); ++j)
            {
                auto offs = scope_stack_[j].this_macro->current_statement;

                const auto& nest = scope_stack_[j].this_macro->copy_nests[offs];
                for (size_t k = 0; k < nest.size(); ++k)
                    res.emplace_back(nest[k].loc,
                        scope_stack_[j],
                        k == 0 ? file_processing_type::MACRO : file_processing_type::COPY,
                        nest[k].member_name);
            }
        }
    }

    return res;
}

location hlasm_context::current_statement_location() const
{
    if (source_stack_.size() > 1 || scope_stack_.size() == 1)
    {
        if (source_stack_.back().copy_stack.size())
        {
            const auto& member = source_stack_.back().copy_stack.back();

            return location(member.current_statement_position(), member.definition_location()->file);
        }
        else
            return source_stack_.back().current_instruction;
    }
    else
    {
        const auto& mac_invo = scope_stack_.back().this_macro;

        return mac_invo->copy_nests[mac_invo->current_statement].back().loc;
    }
}

const std::deque<code_scope>& hlasm_context::scope_stack() const { return scope_stack_; }

const source_context& hlasm_context::current_source() const { return source_stack_.back(); }

const std::vector<copy_member_invocation>& hlasm_context::current_copy_stack() const
{
    return source_stack_.back().copy_stack;
}

std::vector<copy_member_invocation>& hlasm_context::current_copy_stack() { return source_stack_.back().copy_stack; }

const std::vector<copy_member_invocation>&
hlasm_plugin::parser_library::context::hlasm_context::opencode_copy_stack() const
{
    return source_stack_.front().copy_stack;
}

std::vector<copy_member_invocation>& hlasm_plugin::parser_library::context::hlasm_context::opencode_copy_stack()
{
    return source_stack_.front().copy_stack;
}

void hlasm_plugin::parser_library::context::hlasm_context::end_reached()
{
    assert(!m_end_reached);
    m_end_reached = true;

    scope_stack_.erase(scope_stack_.begin() + 1, scope_stack_.end());
    source_stack_.erase(source_stack_.begin() + 1, source_stack_.end());
    auto& opencode = source_stack_.front();
    opencode.copy_stack.clear();
    opencode.proc_stack.erase(opencode.proc_stack.begin() + 1, opencode.proc_stack.end());
}

std::vector<id_index> hlasm_context::whole_copy_stack() const
{
    std::vector<id_index> ret;

    for (auto& entry : source_stack_)
        for (auto& nest : entry.copy_stack)
            ret.push_back(nest.name());

    return ret;
}

void hlasm_context::fill_metrics_files() { metrics.files = visited_files_.size(); }

const hlasm_context::global_variable_storage& hlasm_context::globals() const { return globals_; }

var_sym_ptr hlasm_context::get_var_sym(id_index name) const
{
    const auto* scope = curr_scope();
    if (auto tmp = scope->variables.find(name); tmp != scope->variables.end())
        return tmp->second;

    if (auto s_tmp = scope->system_variables.find(name); s_tmp != scope->system_variables.end())
        return s_tmp->second;

    if (scope->is_in_macro())
    {
        auto m_tmp = scope->this_macro->named_params.find(name);
        if (m_tmp != scope->this_macro->named_params.end())
            return m_tmp->second;
    }

    return var_sym_ptr();
}

void hlasm_context::add_sequence_symbol(sequence_symbol_ptr seq_sym)
{
    auto& opencode = scope_stack_.front();
    opencode.sequence_symbols.try_emplace(seq_sym->name, std::move(seq_sym));
}

const sequence_symbol* hlasm_context::get_sequence_symbol(id_index name) const
{
    const auto* scope = curr_scope();
    label_storage::const_iterator found, end;

    if (scope->is_in_macro())
    {
        found = scope->this_macro->labels.find(name);
        end = scope->this_macro->labels.end();
    }
    else
    {
        found = scope->sequence_symbols.find(name);
        end = scope->sequence_symbols.end();
    }

    if (found != end)
        return found->second.get();
    else
        return nullptr;
}

const sequence_symbol* hlasm_context::get_opencode_sequence_symbol(id_index name) const
{
    if (const auto& sym = scope_stack_.front().sequence_symbols.find(name);
        sym != scope_stack_.front().sequence_symbols.end())
        return sym->second.get();
    return nullptr;
}

void hlasm_context::set_branch_counter(A_t value)
{
    curr_scope()->branch_counter = value;
    ++curr_scope()->branch_counter_change;
}

int hlasm_context::get_branch_counter() const { return curr_scope()->branch_counter; }

void hlasm_context::decrement_branch_counter() { --curr_scope()->branch_counter; }

void hlasm_context::add_mnemonic(id_index mnemo, id_index op_code)
{
    auto tmp = opcode_mnemo_.find(op_code);
    if (tmp != opcode_mnemo_.end())
    {
        if (!tmp->second) // mnemonic was removed
            throw std::invalid_argument("undefined operation code");

        opcode_mnemo_.insert_or_assign(mnemo, tmp->second);
    }
    else
    {
        opcode_t value = { op_code };

        if (auto mac_it = macros_.find(op_code); mac_it != macros_.end())
            value.opcode_detail = mac_it->second;
        else if (auto instr_it = instruction_map_.find(op_code); instr_it != instruction_map_.end())
            value.opcode_detail = instr_it->second;
        else
            throw std::invalid_argument("undefined operation code");

        opcode_mnemo_.insert_or_assign(mnemo, std::move(value));
    }
}

void hlasm_context::remove_mnemonic(id_index mnemo)
{
    if (opcode_mnemo_.find(mnemo) != opcode_mnemo_.end() || is_opcode(mnemo))
        opcode_mnemo_.insert_or_assign(mnemo, opcode_t());
    else
        throw std::invalid_argument("undefined operation code");
}

opcode_t hlasm_context::get_operation_code(id_index symbol) const
{
    if (auto it = opcode_mnemo_.find(symbol); it != opcode_mnemo_.end())
        return it->second;

    opcode_t value;

    if (auto mac_it = macros_.find(symbol); mac_it != macros_.end())
        value = opcode_t { symbol, mac_it->second };
    else if (auto instr_it = instruction_map_.find(symbol); instr_it != instruction_map_.end())
        value = opcode_t { symbol, instr_it->second };

    return value;
}

SET_t hlasm_context::get_attribute_value_ca(
    data_attr_kind attribute, var_sym_ptr var_symbol, std::vector<size_t> offset)
{
    switch (attribute)
    {
        case data_attr_kind::K:
            return var_symbol ? var_symbol->count(std::move(offset)) : 0;
        case data_attr_kind::N:
            return var_symbol ? var_symbol->number(std::move(offset)) : 0;
        case data_attr_kind::T:
            return get_type_attr(var_symbol, std::move(offset));
        default:
            break;
    }

    return SET_t();
}

SET_t hlasm_context::get_attribute_value_ca(data_attr_kind attribute, id_index symbol_name)
{
    if (attribute == data_attr_kind::O)
        return get_opcode_attr(symbol_name);
    return get_attribute_value_ca(attribute, ord_ctx.get_symbol(symbol_name));
}

SET_t hlasm_context::get_attribute_value_ca(data_attr_kind attribute, const symbol* symbol)
{
    switch (attribute)
    {
        case data_attr_kind::D:
            if (symbol)
                return 1;
            return 0;
        case data_attr_kind::T:
            if (symbol)
            {
                auto attr_val = symbol->attributes().get_attribute_value(attribute);
                return std::string { (char)ebcdic_encoding::e2a[attr_val] };
            }
            return "U";
        case data_attr_kind::O:
            if (symbol)
                return get_opcode_attr(symbol->name);
            return "U";
        default:
            if (symbol)
                return symbol->attributes().get_attribute_value(attribute);
            return symbol_attributes::default_value(attribute);
    }
}

C_t hlasm_context::get_type_attr(var_sym_ptr var_symbol, const std::vector<size_t>& offset)
{
    if (!var_symbol)
        return "U";

    C_t value;

    if (auto set_sym = var_symbol->access_set_symbol_base())
    {
        if (set_sym->type != SET_t_enum::C_TYPE)
            return "N";

        auto setc_sym = set_sym->access_set_symbol<C_t>();
        if (offset.empty())
            value = setc_sym->get_value();
        else
            value = setc_sym->get_value(offset.front() - 1);
    }
    else if (auto mac_par = var_symbol->access_macro_param_base())
    {
        auto data = mac_par->get_data(offset);

        while (dynamic_cast<const context::macro_param_data_composite*>(data))
            data = data->get_ith(0);

        value = data->get_value();
    }

    if (value.empty())
        return "O";

    value = expressions::ca_symbol_attribute::try_extract_leading_symbol(value);

    auto res = expressions::ca_constant::try_self_defining_term(value);
    if (res)
        return "N";

    id_index symbol_name = ids().add(std::move(value));
    auto tmp_symbol = ord_ctx.get_symbol(symbol_name);

    if (tmp_symbol)
        return { (char)ebcdic_encoding::e2a[tmp_symbol->attributes().type()] };

    return "U";
}

struct opcode_attr_visitor
{
    std::string operator()(const assembler_instruction*) const { return "A"; }
    std::string operator()(const ca_instruction*) const { return "A"; }
    std::string operator()(const mnemonic_code*) const { return "E"; }
    std::string operator()(const machine_instruction*) const { return "O"; }
    template<typename T>
    std::string operator()(const T&) const
    {
        return "U";
    }
};

C_t hlasm_context::get_opcode_attr(id_index symbol)
{
    auto it = instruction_map_.find(symbol);

    auto mac_it = macros_.find(symbol);

    if (mac_it != macros_.end())
        return "M";

    if (it != instruction_map_.end())
    {
        auto& [opcode, detail] = *it;
        return std::visit(opcode_attr_visitor(), detail);
    }

    return "U";
}

macro_def_ptr hlasm_context::add_macro(id_index name,
    id_index label_param_name,
    std::vector<macro_arg> params,
    statement_block definition,
    copy_nest_storage copy_nests,
    label_storage labels,
    location definition_location,
    std::unordered_set<copy_member_ptr> used_copy_members)
{
    auto result = std::make_shared<macro_definition>(name,
        label_param_name,
        std::move(params),
        std::move(definition),
        std::move(copy_nests),
        std::move(labels),
        std::move(definition_location),
        std::move(used_copy_members));
    add_macro(result);
    return result;
}

void hlasm_context::add_macro(macro_def_ptr macro)
{
    const auto& m = macros_[macro->id] = std::move(macro);
    // associate mnemonic if previously deleted by OPSYN
    if (auto m_op = opcode_mnemo_.find(m->id); m_op != opcode_mnemo_.end() && !m_op->second)
        m_op->second = opcode_t { m->id, m };
};

const hlasm_context::macro_storage& hlasm_context::macros() const { return macros_; }

const hlasm_context::opcode_map& hlasm_context::opcode_mnemo_storage() const { return opcode_mnemo_; }

macro_def_ptr hlasm_context::get_macro_definition(id_index name) const
{
    macro_def_ptr macro_def;

    if (auto mnem = opcode_mnemo_.find(name);
        mnem != opcode_mnemo_.end() && std::holds_alternative<context::macro_def_ptr>(mnem->second.opcode_detail))
        macro_def = std::get<context::macro_def_ptr>(mnem->second.opcode_detail);
    else
    {
        auto tmp = macros_.find(name);
        macro_def = tmp == macros_.end() ? nullptr : tmp->second;
    }

    return macro_def;
}

bool hlasm_context::is_in_macro() const { return scope_stack_.back().is_in_macro(); }

macro_invo_ptr hlasm_context::enter_macro(id_index name, macro_data_ptr label_param_data, std::vector<macro_arg> params)
{
    assert(SYSNDX_ <= SYSNDX_limit);

    macro_def_ptr macro_def = get_macro_definition(name);
    assert(macro_def);

    auto invo = macro_def->call(std::move(label_param_data), std::move(params), ids().add("SYSLIST"));
    auto& new_scope = scope_stack_.emplace_back(invo, macro_def);
    add_system_vars_to_scope(new_scope);
    add_global_system_vars(new_scope);

    visited_files_.insert(macro_def->definition_location.file);

    ++SYSNDX_;

    return invo;
}

void hlasm_context::leave_macro() { scope_stack_.pop_back(); }

macro_invo_ptr hlasm_context::this_macro() const
{
    if (is_in_macro())
        return curr_scope()->this_macro;
    return macro_invo_ptr();
}

const std::string& hlasm_context::opencode_file_name() const { return opencode_file_name_; }

const std::set<std::string>& hlasm_context::get_visited_files() { return visited_files_; }

copy_member_ptr hlasm_context::add_copy_member(
    id_index member, statement_block definition, location definition_location)
{
    auto& copydef = copy_members_[member];
    if (!copydef)
        copydef = std::make_shared<copy_member>(member, std::move(definition), definition_location);
    visited_files_.insert(std::move(definition_location.file));

    return copydef;
}

void hlasm_context::add_copy_member(copy_member_ptr member)
{
    visited_files_.insert(member->definition_location.file);
    copy_members_[member->name] = std::move(member);
}


copy_member_ptr hlasm_context::get_copy_member(id_index member) const
{
    if (auto it = copy_members_.find(member); it != copy_members_.end())
        return it->second;
    return nullptr;
}

void hlasm_context::enter_copy_member(id_index member_name)
{
    auto tmp = copy_members_.find(member_name);
    if (tmp == copy_members_.end())
        throw std::runtime_error("unknown copy member");

    const auto& [name, member] = *tmp;

    source_stack_.back().copy_stack.emplace_back(copy_member_invocation(member));
}

const hlasm_context::copy_member_storage& hlasm_context::copy_members() { return copy_members_; }

void hlasm_context::leave_copy_member() { source_stack_.back().copy_stack.pop_back(); }

void hlasm_context::add_preprocessor_dependency(const std::string& file) { visited_files_.emplace(file); }

void hlasm_context::apply_source_snapshot(source_snapshot snapshot)
{
    assert(std::transform_reduce(source_stack_.begin(),
               source_stack_.end(),
               (size_t)0,
               std::plus {},
               [](const auto& source) { return source.proc_stack.size(); })
        == 1);

    source_stack_.back().current_instruction = std::move(snapshot.instruction);
    source_stack_.back().begin_index = snapshot.begin_index;
    source_stack_.back().end_index = snapshot.end_index;
    source_stack_.back().end_line = snapshot.end_line;

    source_stack_.back().copy_stack.clear();

    for (auto& frame : snapshot.copy_frames)
    {
        copy_member_invocation invo(copy_members_.at(frame.copy_member));
        invo.current_statement = frame.statement_offset;
        source_stack_.back().copy_stack.push_back(std::move(invo));
    }
}

const code_scope& hlasm_context::current_scope() const { return *curr_scope(); }



void hlasm_context::using_add(id_index label,
    std::unique_ptr<expressions::mach_expression> begin,
    std::unique_ptr<expressions::mach_expression> end,
    std::vector<std::unique_ptr<expressions::mach_expression>> bases,
    dependency_evaluation_context eval_ctx,
    processing_stack_t stack)
{
    m_active_usings.back() = m_usings->add(m_active_usings.back(),
        label,
        std::move(begin),
        std::move(end),
        std::move(bases),
        std::move(eval_ctx),
        std::move(stack));
}

void hlasm_context::using_remove(std::vector<std::unique_ptr<expressions::mach_expression>> bases,
    dependency_evaluation_context eval_ctx,
    processing_stack_t stack)
{
    m_active_usings.back() = bases.empty()
        ? m_usings->remove_all()
        : m_usings->remove(m_active_usings.back(), std::move(bases), std::move(eval_ctx), std::move(stack));
}

void hlasm_context::using_push() { m_active_usings.push_back(m_active_usings.back()); }

bool hlasm_context::using_pop()
{
    if (m_active_usings.size() == 1)
        return false;

    m_active_usings.pop_back();
    return true;
}

void hlasm_context::using_resolve(diagnostic_s_consumer& diag) { m_usings->resolve_all(ord_ctx, diag); }

index_t<using_collection> hlasm_context::using_current() const { return m_active_usings.back(); }

hlasm_context::name_result hlasm_context::try_get_symbol_name(const std::string& symbol)
{
    if (symbol.empty() || symbol.size() > 63 || isdigit((unsigned char)symbol.front()))
        return std::make_pair(false, context::id_storage::empty_id);

    for (const auto& c : symbol)
        if (!lexing::lexer::ord_char(c))
            return std::make_pair(false, context::id_storage::empty_id);

    return std::make_pair(true, ids().add(symbol));
}

} // namespace hlasm_plugin::parser_library::context
