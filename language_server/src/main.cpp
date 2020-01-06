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

#include "dispatcher.h"
#include "lsp/lsp_server.h"
#include "dap/dap_server.h"
#include "shared/workspace_manager.h"
#include "logger.h"

#ifdef _WIN32 //set binary mode for input on windows
# include <io.h>
# include <fcntl.h>
# define SET_BINARY_MODE(handle) _setmode(_fileno(handle), O_BINARY)
#else
#define SET_BINARY_MODE(handle)
#endif
//no need for binary on linux, because it does not change \n into \r\n

uint16_t dap_port = 4745;

using namespace hlasm_plugin::language_server;

struct colon_is_space : std::ctype<char> {
	colon_is_space() : std::ctype<char>(get_table()) {}
	static mask const* get_table()
	{
		static mask rc[table_size];
		rc[(unsigned char)'\n'] = std::ctype_base::space;
		return rc;
	}
};

void start_dap(hlasm_plugin::parser_library::workspace_manager * ws_mngr, request_manager * req_mngr)
{	
	asio::io_service io_service_;
	asio::ip::tcp::acceptor acceptor(io_service_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), (uint16_t)dap_port));
	
	for (;;)
	{
		asio::ip::tcp::iostream stream;

		dap::server server(*ws_mngr);

		acceptor.accept(stream.socket());
		
		stream.imbue(std::locale(stream.getloc(), new colon_is_space));

		dispatcher dap_dispatcher(stream, stream, server, *req_mngr);
		dap_dispatcher.run_server_loop();

		stream.close();
	}
}

int main(int argc, char ** argv) {
	using namespace std;
	using namespace hlasm_plugin::language_server;
	
	std::atomic<bool> cancel = false;
	try {
		SET_BINARY_MODE(stdin);
		SET_BINARY_MODE(stdout);

		hlasm_plugin::parser_library::workspace_manager ws_mngr(&cancel);
		request_manager req_mngr(&cancel);

		std::thread dap_th(start_dap, &ws_mngr, &req_mngr);


		cin.imbue(locale(cin.getloc(), new colon_is_space));
		
		lsp::server server(ws_mngr);
		int ret;

		if (argc == 3 && strcmp(argv[1], "-p") == 0)
		{
			//listen on tcp port for lsp client
			int port = atoi(argv[2]);
			if (port <= 0 || port > 65535)
			{
				std::cout << "Wrong port entered.";
				return 1;
			}

			asio::io_service io_service_;
			asio::ip::tcp::acceptor acceptor_(io_service_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), (uint16_t) port));
			asio::ip::tcp::socket socket_(io_service_);
			asio::ip::tcp::iostream stream;
			acceptor_.accept(stream.socket());
			
			stream.imbue(locale(stream.getloc(), new colon_is_space));
			dispatcher lsp_dispatcher(stream, stream, server, req_mngr, &cancel);
			ret = lsp_dispatcher.run_server_loop();
			stream.close();

		}
		else
		{
			//communicate with standard IO
			dispatcher lsp_dispatcher(std::cin, std::cout, server, req_mngr, &cancel);
			ret = lsp_dispatcher.run_server_loop();
		}
		
		
		dap_th.join();

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

