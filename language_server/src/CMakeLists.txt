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

target_sources(language_server_base PRIVATE
	base_protocol_channel.cpp
	base_protocol_channel.h
	blocking_queue.h
	common_types.h
	dispatcher.cpp
	dispatcher.h
	feature.cpp
	feature.h
	json_channel.h
	json_queue_channel.cpp
	json_queue_channel.h
	logger.cpp
	logger.h
	message_router.cpp
	message_router.h
	request_manager.cpp
	request_manager.h
	scope_exit.h
	server.cpp
	server.h
	server_streams.h
	stream_helper.h
)

if(EMSCRIPTEN)
	target_sources(language_server_base PRIVATE emscripten_server_streams.cpp)
else()
	target_sources(language_server_base PRIVATE native_server_streams.cpp)
endif()

target_sources(language_server PRIVATE 
	main.cpp
)

add_subdirectory(dap)
add_subdirectory(lsp)
