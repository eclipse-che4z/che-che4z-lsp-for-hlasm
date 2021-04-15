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

#ifndef PROCESSING_COPY_PROCESSING_INFO_H
#define PROCESSING_COPY_PROCESSING_INFO_H

#include "context/hlasm_statement.h"
#include "context/id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

// data to start copy_processor
struct copy_start_data
{
    context::id_index member_name;
    std::string member_file;
};

// result of copy_processor
struct copy_processing_result
{
    context::statement_block definition;
    location definition_location;
    context::id_index member_name;

    bool invalid_member;
};

} // namespace processing
} // namespace parser_library
} // namespace hlasm_plugin
#endif
