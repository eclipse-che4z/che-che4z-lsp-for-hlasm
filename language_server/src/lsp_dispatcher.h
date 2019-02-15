#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_LSPDISPATCHER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_LSPDISPATCHER_H

#include <iostream>

#include "server.h"

namespace hlasm_plugin {
namespace language_server {

class lsp_dispatcher
{
public:
	lsp_dispatcher(std::ostream & out, server & server);

	int run_server_loop(std::istream & in);
	static bool read_message(std::istream & in, std::string & out);
	static bool read_line(std::istream & in, std::string &out);

	void write_message(const std::string & in);

	void reply(id  id, const Json & result);
	void reply_error(id id, const error & result);
	void notify(const std::string & method, const Json & result);
private:
	server & server_;
	std::ostream & out_;
};

}//namespace language_server
}//namespace hlasm_plugin

#endif
