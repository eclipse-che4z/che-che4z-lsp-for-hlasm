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

#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_LANGUAGEFEATURES_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_LANGUAGEFEATURES_H

#include <vector>

#include "../feature.h"
#include "../logger.h"
#include "protocol.h"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server::lsp {

// a feature that implements definition, references and completion
class feature_language_features : public feature
{
public:
    feature_language_features(parser_library::workspace_manager& ws_mngr, response_provider& response_provider);

    void register_methods(std::map<std::string, method>& methods) override;
    json register_capabilities() override;
    void initialize_feature(const json& initialise_params) override;

private:
    void definition(const json& id, const json& params);
    void references(const json& id, const json& params);
    void hover(const json& id, const json& params);
    void completion(const json& id, const json& params);
    void semantic_tokens(const json& id, const json& params);
    void document_symbol(const json& id, const json& params);

    static json get_markup_content(std::string_view content);
    json document_symbol_children_json(hlasm_plugin::parser_library::document_symbol_item symbol);
    json document_symbol_item_json(hlasm_plugin::parser_library::document_symbol_item symbol);
    json document_symbol_list_json(hlasm_plugin::parser_library::document_symbol_list symbol_list);
};

} // namespace hlasm_plugin::language_server::lsp

#endif
