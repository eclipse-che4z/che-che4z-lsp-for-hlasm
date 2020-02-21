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

#include "tcp_handler.h"

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;
using namespace hlasm_plugin::language_server::dap;

tcp_handler::tcp_handler(parser_library::workspace_manager& ws_mngr, request_manager& req_mngr, uint16_t dap_port)
	: acceptor_(io_service_, asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), dap_port)), ws_mngr_(ws_mngr), req_mngr_(req_mngr) {}

void tcp_handler::handle_accept(const asio::error_code& error)
{
	if (!error)
	{
		dap::server server(ws_mngr_);
		dispatcher dap_dispatcher(*stream_, *stream_, server, req_mngr_);
		dap_dispatcher.run_server_loop();
	}

	async_accept();
}

 void tcp_handler::run_dap()
{
	io_service_.run();
}

 void tcp_handler::async_accept()
{
	try
	{
		if (canceled_)
			return;

		if (stream_)
			stream_->close();
		stream_ = std::make_unique<asio::ip::tcp::iostream>();

		newline_is_space::imbue_stream(*stream_);
		acceptor_.async_accept(*stream_->rdbuf(), std::bind(&tcp_handler::handle_accept, this, std::placeholders::_1));
	}
	catch (asio::system_error & e)
	{
		std::string message = "Warning: Macro tracer asio exception. " + std::string(e.what()) + "\n";
		std::cout << message;
		LOG_WARNING(message);
		return;
	}

}

 void tcp_handler::cancel()
{
	std::lock_guard guard(closing_mutex_);
	canceled_ = true;
	try
	{
		io_service_.stop();
		stream_->close();
		acceptor_.cancel();
		acceptor_.close();
	}
	catch (asio::system_error & e)
	{
		std::string message = "Warning: Macro tracer asio exception. " + std::string(e.what());
		std::cout << message;
		LOG_WARNING(message);
	}
}
