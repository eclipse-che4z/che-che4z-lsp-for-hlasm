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
	dap_feature.cpp
	dap_feature.h
	dap_message_wrappers.cpp
	dap_message_wrappers.h
	dap_server.cpp
	dap_server.h
	dap_session.cpp
	dap_session.h
	dap_session_manager.cpp
	dap_session_manager.h
)