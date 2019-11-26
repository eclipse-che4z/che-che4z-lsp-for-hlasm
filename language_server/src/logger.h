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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_LOGGER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_LOGGER_H

#include <fstream>
#include <string>

namespace hlasm_plugin {
namespace language_server {

#ifdef _DEBUG
#	define LOG_ON
#endif

#ifdef LOG_ON
#	define LOG_ERROR(x)    hlasm_plugin::language_server::logger::get_instance().log(x)
#	define LOG_WARNING(x)  hlasm_plugin::language_server::logger::get_instance().log(x)
#	define LOG_INFO(x)     hlasm_plugin::language_server::logger::get_instance().log(x)
#else
#	define LOG_ERROR(x)    hlasm_plugin::language_server::logger::get_instance().log(x)
#	define LOG_WARNING(x)  hlasm_plugin::language_server::logger::get_instance().log(x)
#	define LOG_INFO(x)
#endif

class logger
{
public:

	static logger & get_instance()
	{
		static logger instance;
		return instance;
	}

	void log(const std::string & data);
	void log(const char * data);

protected:
	std::string current_time();
	
private:
	logger();
	~logger();
	std::ofstream file_;
	
};

} //namespace language_server
} //namespace hlasm_plugin
#endif 

