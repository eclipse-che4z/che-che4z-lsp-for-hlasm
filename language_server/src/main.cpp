#include <thread>

#define ASIO_STANDALONE
#include "asio.hpp"
#include "asio/stream_socket_service.hpp"

#include "dispatcher.h"
#include "lsp/lsp_server.h"
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

struct colon_is_space : std::ctype<char> {
	colon_is_space() : std::ctype<char>(get_table()) {}
	static mask const* get_table()
	{
		static mask rc[table_size];
		rc['\n'] = std::ctype_base::space;
		return &rc[0];
	}
};

#include <chrono>
#include <thread>

int main(int argc, char ** argv) {
	using namespace std;
	using namespace hlasm_plugin::language_server;
	
	try {
		SET_BINARY_MODE(stdin);
		SET_BINARY_MODE(stdout);
		
		cin.imbue(locale(cin.getloc(), new colon_is_space));

		hlasm_plugin::parser_library::workspace_manager ws_mngr;
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
			asio::ip::tcp::acceptor acceptor_(io_service_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
			asio::ip::tcp::socket socket_(io_service_);
			asio::ip::tcp::iostream stream;
			acceptor_.accept(stream.socket());
			
			stream.imbue(locale(stream.getloc(), new colon_is_space));
			dispatcher lsp_dispatcher(stream, stream, server);
			server.set_send_message_provider(&lsp_dispatcher);
			ret = lsp_dispatcher.run_server_loop();
			stream.close();

		}
		else
		{
			//communicate with standard IO
			dispatcher lsp_dispatcher(std::cin, std::cout, server);
			server.set_send_message_provider(&lsp_dispatcher);
			ret = lsp_dispatcher.run_server_loop();
		}

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

