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

#ifndef PARSERLIBRARY_LANGUAGESERVER_TCP_HANDLER_H
#define PARSERLIBRARY_LANGUAGESERVER_TCP_HANDLER_H

#include <memory>
#include <atomic>
#include <mutex>

#define ASIO_STANDALONE
#include "asio.hpp"
#include "asio/stream_socket_service.hpp"

#include "../dispatcher.h"
#include "../stream_helper.h"
#include "dap_server.h"
#include "shared/workspace_manager.h"
#include "../logger.h"


namespace hlasm_plugin::language_server::dap
{

class tcp_handler
{
	asio::io_service io_service_;
	asio::ip::tcp::acceptor acceptor_;
	std::unique_ptr<asio::ip::tcp::iostream> stream_;
	hlasm_plugin::parser_library::workspace_manager& ws_mngr_;
	request_manager& req_mngr_;
	std::atomic<bool> canceled_ = false;
	std::mutex closing_mutex_;
public:
	tcp_handler(hlasm_plugin::parser_library::workspace_manager& ws_mngr, request_manager& req_mngr, uint16_t dap_port);

	void handle_accept(const asio::error_code& error);

	void run_dap();

	void async_accept();

	void cancel();

};


}


#endif
