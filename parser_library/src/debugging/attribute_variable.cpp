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

#include "attribute_variable.h"

#include <cassert>
#include <stdexcept>

using namespace hlasm_plugin::parser_library;
using namespace debugging;

attribute_variable::attribute_variable(std::string name, std::string value)
    : name_(std::move(name))
    , value_(std::move(value))
{}

const std::string& attribute_variable::get_name() const { return name_; }

const std::string& attribute_variable::get_value() const { return value_; }

set_type attribute_variable::type() const { return set_type::UNDEF_TYPE; }

bool attribute_variable::is_scalar() const { return true; }

std::vector<variable_ptr> attribute_variable::values() const { return std::vector<variable_ptr>(); }

size_t attribute_variable::size() const { return 0; }
