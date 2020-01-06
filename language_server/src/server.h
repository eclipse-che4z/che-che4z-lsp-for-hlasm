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

#ifndef HLASMPLUGIN_LANGUAGESERVER_SERVER
#define HLASMPLUGIN_LANGUAGESERVER_SERVER

#include <unordered_set>

#include "feature.h"
#include "common_types.h"
#include "shared/workspace_manager.h"

namespace hlasm_plugin::language_server
{

class send_message_provider
{
public:
	virtual void reply(const json & result) = 0;
};

class server : public response_provider
{

public:
	server(parser_library::workspace_manager & ws_mngr);

	virtual void message_received(const json & message) = 0;

	bool is_shutdown_request_received();
	bool is_exit_notification_received();

	void set_send_message_provider(send_message_provider* provider);
protected:
	send_message_provider * send_message_ = nullptr;

	std::vector<std::unique_ptr<feature> > features_;

	std::map<std::string, method> methods_;

	bool shutdown_request_received_ = false;
	bool exit_notification_received_ = false;

	parser_library::workspace_manager & ws_mngr_;

	virtual void register_methods();


	void call_method(const std::string & method, const json & id, const json & args);
};

}
#endif
