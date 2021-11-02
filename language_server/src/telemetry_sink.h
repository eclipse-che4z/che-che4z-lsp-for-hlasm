/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TELEMETRY_SINK_H
#define HLASMPLUGIN_LANGUAGESERVER_TELEMETRY_SINK_H

#include "telemetry_info.h"


namespace hlasm_plugin::language_server {

class telemetry_sink
{
public:
    virtual void send_telemetry(const telemetry_message& message) = 0;

protected:
    ~telemetry_sink() = default;
};

} // namespace hlasm_plugin::language_server

#endif
