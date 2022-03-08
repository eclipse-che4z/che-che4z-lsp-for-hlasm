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

#ifndef CONTEXT_MACRO_PARAM_DATA_H
#define CONTEXT_MACRO_PARAM_DATA_H

#include <memory>
#include <vector>

#include "common_types.h"

namespace hlasm_plugin::parser_library::context {

class macro_param_data_component;
using macro_data_ptr = std::unique_ptr<macro_param_data_component>;
using macro_data_shared_ptr = std::shared_ptr<macro_param_data_component>;

// base class for data of macro parameters
// data in macro parameters are immutable
class macro_param_data_component
{
public:
    // gets value of current data, composite or simple
    virtual const C_t& get_value() const = 0;
    // gets value of the idx-th value, when exceeds size of data, returns default value
    virtual const macro_param_data_component* get_ith(size_t idx) const = 0;

    // dummy data returning default value everytime
    static const macro_data_shared_ptr dummy;

    // number of components in the object
    const size_t number;

    virtual size_t size() const = 0;

    virtual ~macro_param_data_component();

protected:
    macro_param_data_component(size_t number);
};

// dummy macro data class returning default value everytime
class macro_param_data_dummy : public macro_param_data_component
{
public:
    macro_param_data_dummy();

    // gets default value ("")
    const C_t& get_value() const override;

    // gets this dummy
    const macro_param_data_component* get_ith(size_t idx) const override;

    size_t size() const override;
};

// class representing data of macro parameters holding only single string (=C_t)
class macro_param_data_single : public macro_param_data_component
{
    const C_t data_;

public:
    // returns whole data, here the only string
    const C_t& get_value() const override;

    // gets value of the idx-th value, when exceeds size of data, returns default value
    // get_ith(0) returns this to mimic HLASM
    const macro_param_data_component* get_ith(size_t idx) const override;

    size_t size() const override;

    macro_param_data_single(C_t value);
};

// class representing data of macro parameters holding more nested data
class macro_param_data_composite : public macro_param_data_component
{
    const std::vector<macro_data_ptr> data_;
    mutable C_t value_;

public:
    // returns data of all nested classes in brackets separated by comma
    const C_t& get_value() const override;

    // gets value of the idx-th value, when exceeds size of data, returns default value
    const macro_param_data_component* get_ith(size_t idx) const override;

    size_t size() const override;

    macro_param_data_composite(std::vector<macro_data_ptr> value);
};

// class representing data of macro parameters holding only single dynamic string (=C_t)
class macro_param_data_single_dynamic : public macro_param_data_single
{
public:
    // returns whole data, here the only string
    virtual const C_t& get_value() const override = 0;

    macro_param_data_single_dynamic()
        : macro_param_data_single("")
    {}
};

} // namespace hlasm_plugin::parser_library::context
#endif
