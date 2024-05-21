/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_UTILS_ASYNC_BUSY_WAIT_H
#define HLASMPLUGIN_UTILS_ASYNC_BUSY_WAIT_H

#include "task.h"

namespace hlasm_plugin::utils {
struct
{
    template<typename Channel, typename T>
    value_task<T> operator()(Channel channel, T* result) const
    {
        while (!channel.resolved())
            co_await task::suspend();

        co_return std::move(*result);
    }
} static constexpr async_busy_wait = {};
} // namespace hlasm_plugin::utils

#endif
