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

#include "variable.h"

using namespace hlasm_plugin::parser_library::debugging;

variable::variable(const std::string& name, const std::string& value)
    : name_(name)
    , value_(value)
{}

const std::string& variable::get_value() const { return value_; }

const std::string& variable::get_name() const { return name_; }
