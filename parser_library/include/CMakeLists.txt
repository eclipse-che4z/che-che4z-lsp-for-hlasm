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
#

target_sources(parser_library PUBLIC
    branch_info.h
    completion_item.h
    completion_trigger_kind.h
    debug_types.h
    debugger.h
    diagnostic.h
    document_symbol_item.h
    external_configuration_requests.h
    fade_messages.h
    folding_range.h
    lib_config.h
    location.h
    message_consumer.h
    protocol.h
    range.h
    workspace_manager.h
    workspace_manager_external_file_requests.h
    workspace_manager_requests.h
    workspace_manager_response.h
)
