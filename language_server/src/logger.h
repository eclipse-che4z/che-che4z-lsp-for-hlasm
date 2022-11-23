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

#include <mutex>
#include <string_view>

namespace hlasm_plugin::language_server {

// LOG_INFO is disabled in release, because it writes vast amount of text
// which takes significant time
#ifdef _DEBUG
#    define LOG_ON
#endif


#ifdef LOG_ON
#    define LOG_ERROR(x) hlasm_plugin::language_server::logger::get_instance().log(x)
#    define LOG_WARNING(x) hlasm_plugin::language_server::logger::get_instance().log(x)
#    define LOG_INFO(x) hlasm_plugin::language_server::logger::get_instance().log(x)
#else
#    define LOG_ERROR(x) hlasm_plugin::language_server::logger::get_instance().log(x)
#    define LOG_WARNING(x)
#    define LOG_INFO(x)
#endif

class logger
{
public:
    // Gets singleton instance of logger.
    static logger& get_instance()
    {
        static logger instance;
        return instance;
    }

    // Writes a message to log file.
    void log(std::string_view data);

private:
    // File to write the log into.
    std::mutex mutex_;
};

} // namespace hlasm_plugin::language_server
#endif
