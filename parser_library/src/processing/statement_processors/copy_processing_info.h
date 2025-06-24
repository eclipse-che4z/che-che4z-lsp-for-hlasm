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
#include "context/id_index.h"
#include "location.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library::processing {

// data to start copy_processor
struct copy_start_data
{
    context::id_index member_name;
    utils::resource::resource_location member_loc;
};

// result of copy_processor
struct copy_processing_result
{
    context::statement_block definition;
    location definition_location;
    context::id_index member_name;

    bool invalid_member;
};

} // namespace hlasm_plugin::parser_library::processing
#endif
