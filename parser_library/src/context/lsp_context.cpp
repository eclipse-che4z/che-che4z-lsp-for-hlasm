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

#include "lsp_context.h"

#include "ebcdic_encoding.h"

using namespace hlasm_plugin::parser_library::context;

size_t definition::hash() const { return (size_t)name; }

std::vector<std::string> definition::get_value() const { return { "Defined in file: " + *file_name }; }

bool definition::operator==(const definition& other) const { return name == other.name; }

void hlasm_plugin::parser_library::context::instr_definition::init(
    const std::string* file, const std::string* n, const range& r)
{
    file_name = file;
    name = n;
    definition_range = r;
    version = 0;
}

void instr_definition::clear(const std::string* empty_string)
{
    name = empty_string;
    file_name = empty_string;
    version = 0;
}

std::vector<std::string> var_definition::get_value() const
{
    std::vector<std::string> result;
    switch (type)
    {
        case context::var_type::STRING:
            result.push_back("string");
            break;
        case context::var_type::BOOL:
            result.push_back("bool");
            break;
        case context::var_type::NUM:
            result.push_back("number");
            break;
        case context::var_type::MACRO:
            result.push_back("Macro param");
            break;
    }

    return result;
}

size_t var_definition::hash() const
{
    size_t name_pointer_num = (size_t)name;
    size_t scope_pointer_num = (size_t)scope.name;
    return (name_pointer_num * PRIME1 + scope_pointer_num) * PRIME2 + scope.version;
}

bool var_definition::operator==(const var_definition& other) const
{
    return name == other.name && scope == other.scope;
}

std::vector<std::string> ord_definition::get_value() const
{
    if (val.value_kind() == symbol_value_kind::UNDEF || !attr.has_value())
        return { { definition::get_value() } };

    std::vector<std::string> result;
    if (val.value_kind() == context::symbol_value_kind::ABS)
    {
        result = { std::to_string(val.get_abs()) };
        result.push_back("Absolute Symbol");
    }
    else if (val.value_kind() == context::symbol_value_kind::RELOC)
    {
        result = { val.get_reloc().to_string() };
        result.push_back("Relocatable Symbol");
    }
    // extract its attributes
    if (attr->is_defined(context::data_attr_kind::L))
        result.push_back("L: " + std::to_string(attr->get_attribute_value(context::data_attr_kind::L)));
    if (attr->is_defined(context::data_attr_kind::I))
        result.push_back("I: " + std::to_string(attr->get_attribute_value(context::data_attr_kind::I)));
    if (attr->is_defined(context::data_attr_kind::S))
        result.push_back("S: " + std::to_string(attr->get_attribute_value(context::data_attr_kind::S)));
    if (attr->is_defined(context::data_attr_kind::T))
        result.push_back(
            "T: " + ebcdic_encoding::to_ascii((unsigned char)attr->get_attribute_value(context::data_attr_kind::T)));

    return result;
}

size_t ord_definition::hash() const { return (size_t)name; }

bool ord_definition::operator==(const ord_definition& other) const { return name == other.name; }

std::vector<std::string> instr_definition::get_value() const
{
    if (!item.has_value())
        return { { "" } };

    std::vector<std::string> result = { item->detail };
    if (version != -1)
        result.push_back("version " + std::to_string(version));
    auto doc = item->get_contents();
    result.insert(result.end(), std::make_move_iterator(doc.begin()), std::make_move_iterator(doc.end()));
    return result;
}

size_t instr_definition::hash() const { return (size_t)name * PRIME1 + version; }

bool instr_definition::operator==(const instr_definition& other) const
{
    return name == other.name && version == other.version;
}

bool macro_id::operator==(const macro_id& other) const { return name == other.name && version == other.version; }

size_t seq_definition::hash() const
{
    size_t name_pointer_num = (size_t)name;
    size_t scope_pointer_num = (size_t)scope.name;
    return (name_pointer_num * PRIME1 + scope_pointer_num) * PRIME2 + scope.version;
}

std::vector<std::string> seq_definition::get_value() const
{
    return { "Defined at line " + std::to_string(definition_range.start.line) };
}

bool seq_definition::operator==(const seq_definition& other) const
{
    return name == other.name && scope == other.scope;
}

completion_item_s::completion_item_s(
    std::string label, std::string detail, std::string insert_text, content_pos contents)
    : content_meta(contents)
    , label(std::move(label))
    , detail(std::move(detail))
    , insert_text(std::move(insert_text))
{}

completion_item_s::completion_item_s(
    std::string label, std::string detail, std::string insert_text, std::vector<std::string> contents)
    : content(std::move(contents))
    , label(std::move(label))
    , detail(std::move(detail))
    , insert_text(std::move(insert_text))
{}

std::vector<std::string> completion_item_s::get_contents() const
{
    if (!content_meta.defined)
    {
        std::vector<std::string> result;
        for (size_t i = 1; i <= 10; i++)
        {
            if (content_meta.text->size() <= content_meta.line + i)
                break;
            std::string_view line = content_meta.text->at(content_meta.line + i);
            if (line.size() < 2 || line.front() != '*')
                break;
            line.remove_prefix(1);
            result.push_back(line.data());
        }
        return result;
    }
    return content;
}

void completion_item_s::implode_contents()
{
    if (content_string == "")
    {
        std::stringstream result;
        if (!content_meta.defined)
        {
            for (size_t i = 1; i <= 10; i++)
            {
                if (content_meta.text->size() <= content_meta.line + i)
                    break;
                std::string_view line = content_meta.text->at(content_meta.line + i);
                if (line.size() < 2 || line.front() != '*')
                    break;
                line.remove_prefix(1);
                result << line.data() << '\n';
            }
        }
        else
        {
            for (auto line : content)
                result << line << '\n';
        }
        content_string = result.str();
    }
}
