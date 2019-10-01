#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_DISPATCHER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_DISPATCHER_H

#include <mutex>
#include <iostream>
#include <thread>
#include <deque>
#include <atomic>
#include <condition_variable>

#include "json.hpp"
#include "server.h"

namespace hlasm_plugin {
namespace language_server {

struct request
{
	json message;
	bool valid;
};

class dispatcher : public send_message_provider
{
public:
	dispatcher(std::istream & in, std::ostream & out, server & server, std::atomic<bool> * cancel = nullptr);

	int run_server_loop();
	bool read_message(std::string & out);

	void write_message(const std::string & in);

	void reply(const json & result) override;

private:
	server & server_;
	std::istream & in_;
	std::ostream & out_;

	std::mutex mtx_;

	std::atomic<bool>* cancel_;
	std::thread worker_;
	std::mutex q_mtx_;
	std::condition_variable cond_;
	std::deque<request> requests_;
	std::string currently_running_file_;

	void handle_request_();
	std::string get_request_file_(json r);
};

}//namespace language_server
}//namespace hlasm_plugin

#endif
