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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TEST_SEND_MESSAGE_PROVIDER_MOCK_H
#define HLASMPLUGIN_LANGUAGESERVER_TEST_SEND_MESSAGE_PROVIDER_MOCK_H

#include "../src/server.h"

class send_message_provider_mock : public hlasm_plugin::language_server::send_message_provider
{
public:
	MOCK_METHOD1(reply, void(const json &));
};

#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_SEND_MESSAGE_PROVIDER_MOCK_H
