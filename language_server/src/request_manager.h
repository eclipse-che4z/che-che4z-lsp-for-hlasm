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

#ifndef HLASMPLUGIN_LANGUAGESERVER_REQUEST_MANAGER_H
#define HLASMPLUGIN_LANGUAGESERVER_REQUEST_MANAGER_H
#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>

#include "server.h"


namespace hlasm_plugin::language_server
{

struct request
{
	request(json message, server* executing_server);
	json message;
	bool valid;
	server* executing_server;
};

class request_manager
{
public:
	request_manager(std::atomic<bool>* cancel);

	void add_request(server* server, json message);
	void finish_server_requests(server* server);
	void end_worker();
private:

	std::atomic<bool> end_worker_;
	std::thread worker_;
	std::mutex q_mtx_;
	std::condition_variable cond_;

	std::string currently_running_file_;
	std::atomic<server *> currently_running_server_;
	
	void handle_request_(const std::atomic<bool>* end_loop);
	std::string get_request_file_(json r);

	std::deque<request> requests_;
	std::atomic<bool>* cancel_;
	
};


}

#endif