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

#include "gmock/gmock.h"

#include "../src/feature.h"

using namespace hlasm_plugin::language_server;

class response_provider_mock : public response_provider
{
	MOCK_METHOD3(respond, void(const json & id, const std::string & requested_method, const json & args));
	MOCK_METHOD2(notify, void(const std::string & method, const json & args));
	MOCK_METHOD5(respond_error, void(const json & id, const std::string & requested_method,
		int err_code, const std::string & err_message, const json & error));
};
