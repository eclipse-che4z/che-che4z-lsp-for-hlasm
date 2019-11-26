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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_LSP_SERVER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_LSP_SERVER_H

#include <functional>
#include <memory>
#include <unordered_set>

#include "json.hpp"

#include "shared/workspace_manager.h"
#include "../common_types.h"
#include "../feature.h"
#include "../server.h"

namespace hlasm_plugin::language_server::lsp {

enum class message_type {
	MT_ERROR = 1,
	MT_WARNING = 2,
	MT_INFO = 3,
	MT_LOG = 4
};

class server : public hlasm_plugin::language_server::server, public parser_library::diagnostics_consumer
{

public:
	server(parser_library::workspace_manager & ws_mngr);

	virtual void message_received(const json & message) override;
protected:

	virtual void respond(const json & id, const std::string & requested_method, const json & args) override;
	virtual void notify(const std::string & method, const json & args) override;
	virtual void respond_error(const json & id, const std::string & requested_method, int err_code, const std::string & err_message, const json & error) override;

	virtual void register_methods() override;

private:
	//requests
	void on_initialize(json id, const json & param);
	void on_shutdown(json id, const json & param);

	//notifications
	void on_exit(json id, const json & param);

	//client notifications
	void show_message(const std::string & message, message_type type);

	std::unordered_set<std::string> last_diagnostics_files_;
	virtual void consume_diagnostics(parser_library::diagnostic_list diagnostics) override;
	
};

}//namespace hlasm_plugin::language_server::lsp


#endif
