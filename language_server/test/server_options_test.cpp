/*
 * Copyright (c) 2024 Broadcom.
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

#include "gtest/gtest.h"

#include "server_options.h"

using namespace hlasm_plugin::language_server;

TEST(server_options, extensions)
{
    const char* const opts[] = {
        "--vscode-extensions",
    };

    auto result = parse_options(opts);

    ASSERT_TRUE(result);

    EXPECT_TRUE(result->enable_vscode_extension);
}

TEST(server_options, log_level)
{
    const char* const opts[] = {
        "--log-level=1",
    };

    auto result = parse_options(opts);

    ASSERT_TRUE(result);

    EXPECT_EQ(result->log_level, 1);
}

TEST(server_options, lsp_port)
{
    const char* const opts[] = {
        "--lsp-port=12345",
    };

    auto result = parse_options(opts);

    ASSERT_TRUE(result);

    EXPECT_EQ(result->port, 12345);
}

TEST(server_options, error_extensions)
{
    const char* const opts[] = {
        "--vscode-extension",
    };

    auto result = parse_options(opts);

    EXPECT_FALSE(result);
}

TEST(server_options, error_port_negative)
{
    const char* const opts[] = {
        "--lsp-port=-1",
    };

    auto result = parse_options(opts);

    EXPECT_FALSE(result);
}

TEST(server_options, error_port_too_big)
{
    const char* const opts[] = {
        "--lsp-port=65536",
    };

    auto result = parse_options(opts);

    EXPECT_FALSE(result);
}

TEST(server_options, log_level_too_big)
{
    const char* const opts[] = {
        "--log-level=3",
    };

    auto result = parse_options(opts);

    EXPECT_FALSE(result);
}
