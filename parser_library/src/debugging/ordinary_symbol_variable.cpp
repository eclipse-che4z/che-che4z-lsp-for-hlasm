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

#include <stdexcept>
#include <cassert>

#include "ordinary_symbol_variable.h"
#include "attribute_variable.h"
#include "../ebcdic_encoding.h"

using namespace hlasm_plugin::parser_library;
using namespace debugging;

static const std::string empty_string = "";
static const std::string undef_string = "UNDEF";
static const std::string reloc_string = "RELOC";
static const std::string complex_string = "COMPLEX";

ordinary_symbol_variable::ordinary_symbol_variable(const context::symbol& symbol)
	: symbol_(symbol)
{
	if (symbol_.kind() == context::symbol_value_kind::ABS)
		value_.emplace(std::to_string(symbol_.value().get_abs()));
}

const std::string& ordinary_symbol_variable::get_string_value() const
{
	switch (symbol_.kind())
	{
	case context::symbol_value_kind::ABS:
		assert(false);
		return empty_string;
	case context::symbol_value_kind::RELOC:
		if (symbol_.value().get_reloc().is_complex())
			return complex_string;
		else
			return reloc_string;
	case context::symbol_value_kind::UNDEF:
		return undef_string;
	default:
		return empty_string;
	}
}

set_type ordinary_symbol_variable::type() const
{
	return set_type::UNDEF_TYPE;
}

const std::string& ordinary_symbol_variable::get_string_name() const
{
	return *symbol_.name;
}

bool ordinary_symbol_variable::is_scalar() const
{
	return !(symbol_.attributes().is_defined(context::data_attr_kind::L) ||
		symbol_.attributes().is_defined(context::data_attr_kind::I) ||
		symbol_.attributes().is_defined(context::data_attr_kind::S) ||
		symbol_.attributes().is_defined(context::data_attr_kind::T));
}

std::vector<variable_ptr> ordinary_symbol_variable::values() const
{
	std::vector<std::unique_ptr<variable>> vars;
	if(symbol_.attributes().is_defined(context::data_attr_kind::L))
		vars.emplace_back(std::make_unique<attribute_variable>("L",
			std::to_string(symbol_.attributes().get_attribute_value(context::data_attr_kind::L))
			));
	if (symbol_.attributes().is_defined(context::data_attr_kind::I))
		vars.emplace_back(std::make_unique<attribute_variable>("I",
			std::to_string(symbol_.attributes().get_attribute_value(context::data_attr_kind::I))
			));
	if (symbol_.attributes().is_defined(context::data_attr_kind::S))
		vars.emplace_back(std::make_unique<attribute_variable>("S",
			std::to_string(symbol_.attributes().get_attribute_value(context::data_attr_kind::S))
			));
	if (symbol_.attributes().is_defined(context::data_attr_kind::T))
		vars.emplace_back(std::make_unique<attribute_variable>("T",
			ebcdic_encoding::to_ascii((unsigned char)symbol_.attributes().get_attribute_value(context::data_attr_kind::T))
			));

	return vars;
}

size_t ordinary_symbol_variable::size() const
{
	return (symbol_.attributes().is_defined(context::data_attr_kind::L) ? 1 : 0) +
		(symbol_.attributes().is_defined(context::data_attr_kind::I) ? 1 : 0) +
		(symbol_.attributes().is_defined(context::data_attr_kind::S) ? 1 : 0) +
		(symbol_.attributes().is_defined(context::data_attr_kind::T) ? 1 : 0);
}
