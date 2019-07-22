#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_DISPATCHER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_DISPATCHER_H

#include <mutex>
#include <iostream>

#include "json.hpp"
#include "server.h"

namespace hlasm_plugin {
namespace language_server {

class dispatcher : public send_message_provider
{
public:
	dispatcher(std::istream & in, std::ostream & out, server & server);

	int run_server_loop();
	bool read_message(std::string & out);

	void write_message(const std::string & in);

	void reply(const json & result) override;

private:
	server & server_;
	std::istream & in_;
	std::ostream & out_;

	std::mutex mtx_;
};

}//namespace language_server
}//namespace hlasm_plugin

#endif
