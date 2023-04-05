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

#ifndef HLASMPLUGIN_PARSERLIBRARY_MESSAGE_CONSUMER_H
#define HLASMPLUGIN_PARSERLIBRARY_MESSAGE_CONSUMER_H

namespace hlasm_plugin::parser_library {

enum class message_type
{
    MT_ERROR = 1,
    MT_WARNING = 2,
    MT_INFO = 3,
    MT_LOG = 4
};

class message_consumer
{
public:
    virtual void show_message(const char* message, message_type type) = 0;

protected:
    ~message_consumer() = default;
};

} // namespace hlasm_plugin::parser_library

#endif
