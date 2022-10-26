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

#ifndef HLASMPARSER_PARSERLIBRARY_PROCESSING_PREPROCESSORS_PREPROCESSOR_TYPES_H
#define HLASMPARSER_PARSERLIBRARY_PROCESSING_PREPROCESSORS_PREPROCESSOR_TYPES_H

namespace hlasm_plugin::parser_library::processing {
enum class preprocessor_type
{
    NONE,
    DB2,
    ENDEVOR,
    CICS
};
}

#endif