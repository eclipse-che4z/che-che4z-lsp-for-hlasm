#include <cstdlib>
#include <iostream>

#include <memory>
#include <string>
#include <thread>

#include <fstream>
#include <cstdio>

#include "logger.h"
#include "lsp_dispatcher.h"
#include "server.h"

#ifdef _WIN32
# include <io.h>
# include <fcntl.h>
# define SET_BINARY_MODE(handle, args) _setmode(_fileno(handle), O_BINARY)
#else
//TODO UNIX support for binary files
# define SET_BINARY_MODE(handle, args) freopen(NULL, args, stdin);
#endif



int main() {
	using namespace std;
	using namespace hlasm_plugin::language_server;
	
	SET_BINARY_MODE(stdin, "rb");

	server server;
	lsp_dispatcher dispatcher{ cout, server};
	
	
	int ret = dispatcher.run_server_loop(&cin);

	return ret;
}

