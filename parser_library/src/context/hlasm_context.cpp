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
#include <stdexcept>

#include "ebcdic_encoding.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_symbol_attribute.h"
#include "instruction.h"

namespace hlasm_plugin::parser_library::context {

code_scope* hlasm_context::curr_scope() { return &scope_stack_.back(); }

const code_scope* hlasm_context::curr_scope() const { return &scope_stack_.back(); }

hlasm_context::instruction_storage hlasm_context::init_instruction_map()
{
    hlasm_context::instruction_storage instr_map;
    for (auto& [name, instr] : instruction::machine_instructions)
    {
        auto id = ids().add(name);
        instr_map.emplace(id, &instr);
    }
    for (auto& [name, instr] : instruction::assembler_instructions)
    {
        auto id = ids().add(name);
        instr_map.emplace(id, &instr);
    }
    for (auto& instr : instruction::ca_instructions)
    {
        auto id = ids().add(instr.name);
        instr_map.emplace(id, &instr);
    }
    for (auto& [name, instr] : instruction::mnemonic_codes)
    {
        auto id = ids().add(name);
        instr_map.emplace(id, &instr);
    }
    return instr_map;
}

void hlasm_context::add_system_vars_to_scope()
{
    if (curr_scope()->is_in_macro())
    {
        {
            auto SYSECT = ids().add("SYSECT");

            auto sect_name = ord_ctx.current_section() ? ord_ctx.current_section()->name : id_storage::empty_id;

            macro_data_ptr mac_data = std::make_unique<macro_param_data_single>(*sect_name);

            sys_sym_ptr var = std::make_shared<system_variable>(SYSECT, std::move(mac_data), false);

            curr_scope()->system_variables.insert({ SYSECT, std::move(var) });
        }

        {
            auto SYSNDX = ids().add("SYSNDX");

            std::string value = std::to_string(SYSNDX_);
            if (auto value_len = value.size(); value_len < 4)
                value.insert(0, 4 - value_len, '0');

            macro_data_ptr mac_data = std::make_unique<macro_param_data_single>(std::move(value));

            sys_sym_ptr var = std::make_shared<system_variable>(SYSNDX, std::move(mac_data), false);

            curr_scope()->system_variables.insert({ SYSNDX, std::move(var) });
        }

        {
            // auto SYSSTYP = ids().add("SYSSTYP");

            // macro_data_ptr mac_data = nullptr;

            // if (ord_ctx.current_section())
            //{
            //     switch (ord_ctx.current_section()->kind)
            //     {
            //         case context::section_kind::COMMON:
            //             mac_data = std::make_unique<macro_param_data_single>("COM");
            //             break;
            //         case context::section_kind::DUMMY:
            //             mac_data = std::make_unique<macro_param_data_single>("DSECT");
            //             break;
            //         case context::section_kind::READONLY:
            //             mac_data = std::make_unique<macro_param_data_single>("RSECT");
            //             break;
            //         case context::section_kind::EXECUTABLE:
            //             mac_data = std::make_unique<macro_param_data_single>("CSECT");
            //             break;
            //         default:
            //             break;
            //     }
            // }

            // sys_sym_ptr var = std::make_shared<system_variable>(SYSSTYP, std::move(mac_data), false);

            // curr_scope()->system_variables.insert({ SYSSTYP, std::move(var) });
            //  TODO uncomment when tests are prepared


             auto SYSSTYP = ids().add("SYSSTYP");

             auto val_styp = std::make_shared<set_symbol<C_t>>(SYSSTYP, true, false);
             if (ord_ctx.current_section())
            {
                 switch (ord_ctx.current_section()->kind)
                 {
                     case context::section_kind::COMMON:
                         val_styp->set_value("COM");
                         break;
                     case context::section_kind::DUMMY:
                         val_styp->set_value("DSECT");
                         break;
                     case context::section_kind::READONLY:
                         val_styp->set_value("RSECT");
                         break;
                     case context::section_kind::EXECUTABLE:
                         val_styp->set_value("CSECT");
                         break;
                     default:
                         break;
                 }
             }
             curr_scope()->variables.insert({ SYSSTYP, val_styp });
        }

        {
            auto SYSLOC = ids().add("SYSLOC");

            std::string location_counter_name = "";

            if (ord_ctx.current_section())
            {
                location_counter_name = *ord_ctx.current_section()->current_location_counter().name;
            }

            macro_data_ptr mac_data = std::make_unique<macro_param_data_single>(location_counter_name);

            sys_sym_ptr var = std::make_shared<system_variable>(SYSLOC, std::move(mac_data), false);

            curr_scope()->system_variables.insert({ SYSLOC, std::move(var) });
        }

        {
            auto SYSNEST = ids().add("SYSNEST");

            std::string value = std::to_string(scope_stack_.size() - 1);

            macro_data_ptr mac_data = std::make_unique<macro_param_data_single>(std::move(value));

            sys_sym_ptr var = std::make_shared<system_variable>(SYSNEST, std::move(mac_data), false);

            curr_scope()->system_variables.insert({ SYSNEST, var });
        }

        {
            auto SYSMAC = ids().add("SYSMAC");

            std::vector<macro_data_ptr> data;

            for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it)
            {
                std::string tmp;
                if (it->is_in_macro())
                    tmp = *it->this_macro->id;
                else
                    tmp = "OPEN CODE";
                data.push_back(std::make_unique<macro_param_data_single>(std::move(tmp)));
            }

            macro_data_ptr mac_data = std::make_unique<macro_param_data_composite>(std::move(data));

            sys_sym_ptr var = std::make_shared<system_variable_sysmac>(SYSMAC, std::move(mac_data), false);

            curr_scope()->system_variables.insert({ SYSMAC, std::move(var) });
        }
    }
    add_global_system_vars();
}

void hlasm_context::add_global_system_vars()
{
    auto SYSDATC = ids().add("SYSDATC");
    auto SYSDATE = ids().add("SYSDATE");
    auto SYSTIME = ids().add("SYSTIME");
    auto SYSPARM = ids().add("SYSPARM");
    auto SYSOPT_RENT = ids().add("SYSOPT_RENT");
    auto SYSTEM_ID = ids().add("SYSTEM_ID");

    if (!is_in_macro())
    {
        auto datc = std::make_shared<set_symbol<C_t>>(SYSDATC, true, true);
        auto date = std::make_shared<set_symbol<C_t>>(SYSDATE, true, true);
        auto time = std::make_shared<set_symbol<C_t>>(SYSTIME, true, true);

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

        datc->set_value(std::move(datc_val));

        date_val.append(year.c_str() + 2);
        date->set_value(std::move(date_val));

        globals_.insert({ SYSDATC, datc });
        globals_.insert({ SYSDATE, date });

        std::string time_val;
        if (now->tm_hour < 10)
            time_val.push_back('0');
        time_val.append(std::to_string(now->tm_hour));
        time_val.push_back(':');
        if (now->tm_min < 10)
            time_val.push_back('0');
        time_val.append(std::to_string(now->tm_min));

        time->set_value(std::move(time_val));
        globals_.insert({ SYSTIME, time });

        {
            auto val = std::make_shared<set_symbol<C_t>>(SYSPARM, true, true);

            val->set_value(asm_options_.sysparm);

            globals_.insert({ SYSPARM, std::move(val) });
        }
        {
            auto val = std::make_shared<set_symbol<B_t>>(SYSOPT_RENT, true, true);
            globals_.insert({ SYSOPT_RENT, std::move(val) });
        }
        {
            auto val = std::make_shared<set_symbol<C_t>>(SYSTEM_ID, true, true);

            val->set_value(asm_options_.system_id);

            globals_.insert({ SYSTEM_ID, std::move(val) });
        }
    }

    auto glob = globals_.find(SYSDATC);
    curr_scope()->variables.insert({ glob->second->id, glob->second });
    glob = globals_.find(SYSDATE);
    curr_scope()->variables.insert({ glob->second->id, glob->second });
    glob = globals_.find(SYSTIME);
    curr_scope()->variables.insert({ glob->second->id, glob->second });
    glob = globals_.find(SYSPARM);
    curr_scope()->variables.insert({ glob->second->id, glob->second });
    glob = globals_.find(SYSOPT_RENT);
    curr_scope()->variables.insert({ glob->second->id, glob->second });
    glob = globals_.find(SYSTEM_ID);
    curr_scope()->variables.insert({ glob->second->id, glob->second });
}

bool hlasm_context::is_opcode(id_index symbol) const
{
    return macros_.find(symbol) != macros_.end() || instruction_map_.find(symbol) != instruction_map_.end();
}

hlasm_context::hlasm_context(std::string file_name, asm_option asm_options, std::shared_ptr<id_storage> init_ids)
    : ids_(std::move(init_ids))
    , opencode_file_name_(file_name)
    , asm_options_(std::move(asm_options))
    , instruction_map_(init_instruction_map())
    , ord_ctx(*ids_, *this)
{
    scope_stack_.emplace_back();
    visited_files_.insert(file_name);
    push_statement_processing(processing::processing_kind::ORDINARY, std::move(file_name));
    add_global_system_vars();
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
    assert(!proc_stack_.empty());
    proc_stack_.emplace_back(kind, false);
}

void hlasm_context::push_statement_processing(const processing::processing_kind kind, std::string file_name)
{
    source_stack_.emplace_back(std::move(file_name));

    proc_stack_.emplace_back(kind, true);
}

void hlasm_context::pop_statement_processing()
{
    if (proc_stack_.back().owns_source)
        source_stack_.pop_back();

    proc_stack_.pop_back();
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
    source_stack_.front().copy_stack.clear();
    proc_stack_.erase(proc_stack_.begin() + 1, proc_stack_.end());
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

const code_scope::set_sym_storage& hlasm_context::globals() const { return globals_; }

var_sym_ptr hlasm_context::get_var_sym(id_index name)
{
    auto tmp = curr_scope()->variables.find(name);
    if (tmp != curr_scope()->variables.end())
        return tmp->second;

    auto s_tmp = curr_scope()->system_variables.find(name);
    if (s_tmp != curr_scope()->system_variables.end())
        return s_tmp->second;

    if (curr_scope()->is_in_macro())
    {
        auto m_tmp = curr_scope()->this_macro->named_params.find(name);
        if (m_tmp != curr_scope()->this_macro->named_params.end())
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
    label_storage::const_iterator found, end;

    if (curr_scope()->is_in_macro())
    {
        found = curr_scope()->this_macro->labels.find(name);
        end = curr_scope()->this_macro->labels.end();
    }
    else
    {
        found = curr_scope()->sequence_symbols.find(name);
        end = curr_scope()->sequence_symbols.end();
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
    scope_stack_.emplace_back(invo, macro_def);
    add_system_vars_to_scope();

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
    assert(proc_stack_.size() == 1);

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

} // namespace hlasm_plugin::parser_library::context
