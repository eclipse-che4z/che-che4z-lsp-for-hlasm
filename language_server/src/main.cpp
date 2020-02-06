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

#include <thread>
#include <chrono>

#define ASIO_STANDALONE
#include "asio.hpp"
#include "asio/stream_socket_service.hpp"
#include "asio/system_error.hpp"

#include "dispatcher.h"
#include "lsp/lsp_server.h"
#include "dap/dap_server.h"
#include "shared/workspace_manager.h"
#include "logger.h"
#include "stream_helper.h"
#include "dap/tcp_handler.h"

#ifdef _WIN32 //set binary mode for input on windows
# include <io.h>
# include <fcntl.h>
# define SET_BINARY_MODE(handle) _setmode(_fileno(handle), O_BINARY)
#else
#define SET_BINARY_MODE(handle)
#endif
//no need for binary on linux, because it does not change \n into \r\n

using namespace hlasm_plugin::language_server;

int main(int argc, char ** argv) {
	using namespace std;
	using namespace hlasm_plugin::language_server;
	// user must define at least one port for dap, e.g. "-p 4745"
	if (argc < 3 || strcmp(argv[1], "-p") != 0)
	{
		std::cout << "Invalid arguments. Use language_server -p <debug port> <lsp port>";
		return 1;
	}

	int dap_port, lsp_port;
	dap_port = atoi(argv[2]);
	if (dap_port <= 0 || dap_port > 65535)
	{
		std::cout << "Wrong port entered.";
		return 1;
	}

	std::atomic<bool> cancel = false;
	try {
		SET_BINARY_MODE(stdin);
		SET_BINARY_MODE(stdout);

		hlasm_plugin::parser_library::workspace_manager ws_mngr(&cancel);
		request_manager req_mngr(&cancel);
		dap::tcp_handler dap_handler(ws_mngr, req_mngr, (uint16_t)dap_port);
		dap_handler.async_accept();
		std::thread dap_thread([&dap_handler]() {dap_handler.run_dap(); });

		newline_is_space::imbue_stream(cin);
		
		lsp::server server(ws_mngr);
		int ret;

		if (argc > 3)
		{
			// if second port is defined, it is used for tcp lsp communication
			lsp_port = atoi(argv[3]);
			if (lsp_port <= 0 || lsp_port > 65535)
			{
				std::cout << "Wrong port entered.";
				return 1;
			}

			//setup tcp
			asio::io_service io_service_;
			asio::ip::tcp::acceptor acceptor_(io_service_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), (uint16_t)lsp_port));
			asio::ip::tcp::socket socket_(io_service_);
			asio::ip::tcp::iostream stream;
			acceptor_.accept(stream.socket());
			
			newline_is_space::imbue_stream(stream);

			dispatcher lsp_dispatcher(stream, stream, server, req_mngr);
			ret = lsp_dispatcher.run_server_loop();
			stream.close();

		}
		else
		{
			//communicate with standard IO
			dispatcher lsp_dispatcher(std::cin, std::cout, server, req_mngr);
			ret = lsp_dispatcher.run_server_loop();
		}
		

		dap_handler.cancel();
		dap_thread.join();
		req_mngr.end_worker();

		return ret;
	}
	catch (std::exception& ex)
	{
		LOG_ERROR(ex.what());
		return 1;
	}
	catch (...)
	{
		LOG_ERROR("Unknown error occured. Terminating.");
		return 2;
	}
}

