# Copyright (c) 2021 Broadcom.
# The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
#
# Contributors:
#   Broadcom, Inc. - initial API and implementation

target_sources(hlasm_language_server_base PRIVATE
    base_protocol_channel.cpp
    base_protocol_channel.h
    blocking_queue.h
    external_file_reader.cpp
    external_file_reader.h
    feature.cpp
    feature.h
    json_channel.cpp
    json_channel.h
    json_queue_channel.cpp
    json_queue_channel.h
    logger.cpp
    logger.h
    message_router.cpp
    message_router.h
    parsing_metadata_serialization.cpp
    parsing_metadata_serialization.h
    send_message_provider.h
    server.cpp
    server.h
    server_options.cpp
    server_options.h
    server_streams.h
    stream_helper.h
    telemetry_broker.h
    telemetry_info.h
    telemetry_sink.h
    virtual_file_provider.cpp
    virtual_file_provider.h
)

if(EMSCRIPTEN)
    target_sources(hlasm_language_server_base PRIVATE emscripten_server_streams.cpp)
else()
    target_sources(hlasm_language_server_base PRIVATE native_server_streams.cpp stream_helper.cpp)
endif()

target_sources(hlasm_language_server PRIVATE
    main.cpp
)

add_subdirectory(dap)
add_subdirectory(lsp)
