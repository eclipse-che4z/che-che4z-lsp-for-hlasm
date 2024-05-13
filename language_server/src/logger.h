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

#include <concepts>
#include <mutex>
#include <span>
#include <string_view>

namespace hlasm_plugin::language_server {

#define LOG_WRITE(l, ...)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        const unsigned level = l;                                                                                      \
        if (level > hlasm_plugin::language_server::logger::instance.level())                                           \
            break;                                                                                                     \
        hlasm_plugin::language_server::logger::instance.log(level, __VA_ARGS__);                                       \
    } while (false)

#define LOG_ERROR(...) LOG_WRITE(0, __VA_ARGS__)
#define LOG_WARNING(...) LOG_WRITE(1, __VA_ARGS__)
#define LOG_INFO(...) LOG_WRITE(2, __VA_ARGS__)

class logger
{
#ifdef _DEBUG
    static constexpr unsigned default_log_level = 2;
#else
    static constexpr unsigned default_log_level = 0;
#endif

    unsigned m_level = default_log_level;
    std::mutex m_mutex;

    void log_impl(unsigned level, std::span<std::string_view> args);

public:
    static logger instance;
    static constexpr unsigned max_log_level = 2;

    unsigned level() const noexcept { return m_level; }
    void level(unsigned l) noexcept
    {
        if (l > max_log_level)
            m_level = max_log_level;
        else
            m_level = l;
    }

    template<std::convertible_to<std::string_view>... Args>
    void log(unsigned level, Args&&... args)
    {
        std::string_view args_sv[] { {}, {}, {}, {}, std::string_view(args)... };
        log_impl(level, args_sv);
    }
};

} // namespace hlasm_plugin::language_server
#endif
