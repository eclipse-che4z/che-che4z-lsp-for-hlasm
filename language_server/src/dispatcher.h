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
#include "request_manager.h"

namespace hlasm_plugin {
namespace language_server {

class dispatcher : public send_message_provider
{
public:
	dispatcher(std::istream & in, std::ostream & out, server & server, request_manager & req_mngr);

	int run_server_loop();
	bool read_message(std::string & out);

	void write_message(const std::string & in);

	void reply(const json & result) override;

private:
	server & server_;
	std::istream & in_;
	std::ostream & out_;

	std::mutex mtx_;

	request_manager& req_mngr_;

	
};

}//namespace language_server
}//namespace hlasm_plugin

#endif
