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

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;

#include "gtest/gtest.h"

#include "../compare_unordered_vectors.h"
#include "analyzer_fixture.h"

struct lsp_context_macro_documentation : public analyzer_fixture
{
    const static inline std::string opencode =
        R"(
       MACRO
       MAC     &FIRST_PARAM,      first param remark                   X
               &SECOND_PARAM=1    second param remark
* After macro line 1
.* After macro line 2
       MEND

       MAC
 
)";


    lsp_context_macro_documentation()
        : analyzer_fixture(input)
    {}
};
