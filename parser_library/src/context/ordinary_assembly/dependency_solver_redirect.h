/*
 * Copyright (c) 2022 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CONTEXT_DEPENDENCY_SOLVER_REDIRECT_H
#define HLASMPLUGIN_PARSERLIBRARY_CONTEXT_DEPENDENCY_SOLVER_REDIRECT_H

#include "dependable.h"

namespace hlasm_plugin::parser_library::context {

class dependency_solver_redirect : public dependency_solver
{
    dependency_solver* m_base;

public:
    const symbol* get_symbol(id_index name) const override;
    std::optional<address> get_loctr() const override;
    id_index get_literal_id(const std::shared_ptr<const expressions::data_definition>& lit) override;
    bool using_active(id_index label, const section* sect) const override;
    using_evaluate_result using_evaluate(
        id_index label, const section* owner, int32_t offset, bool long_offset) const override;
    std::variant<const symbol*, symbol_candidate> get_symbol_candidate(id_index name) const override;
    std::string get_opcode_attr(id_index symbol) const override;

protected:
    explicit dependency_solver_redirect(dependency_solver& base)
        : m_base(&base)
    {}

    ~dependency_solver_redirect() = default;
};

} // namespace hlasm_plugin::parser_library::context

#endif
