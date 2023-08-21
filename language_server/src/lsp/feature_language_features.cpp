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


#include "feature_language_features.h"

#include <functional>
#include <stack>
#include <utility>

#include "../feature.h"
#include "nlohmann/json.hpp"
#include "utils/error_codes.h"
#include "utils/resource_location.h"
#include "workspace_manager_response.h"

namespace hlasm_plugin::language_server::lsp {

namespace {
using namespace hlasm_plugin::parser_library;
std::string extract_document_uri(const nlohmann::json& j)
{
    auto doc = j.find("textDocument");
    if (doc == j.end() || !doc->is_object())
        return {};
    auto uri = doc->find("uri");
    if (uri == doc->end() || !uri->is_string())
        return {};

    return uri->get<std::string>();
}
position extract_position(const nlohmann::json& j)
{
    auto pos = j.find("position");
    if (pos == j.end() || !pos->is_object())
        return {};
    auto line = pos->find("line");
    auto character = pos->find("character");
    if (line == pos->end() || character == pos->end() || !line->is_number() || !character->is_number())
        return {};

    return position(line->get<int>(), character->get<int>());
}

auto extract_trigger(const nlohmann::json& j)
{
    std::pair<completion_trigger_kind, char> result(completion_trigger_kind::invoked, '\0');

    auto context = j.find("context");
    if (context == j.end() || !context->is_object())
        return result;

    auto triggerKind = context->find("triggerKind");
    if (triggerKind == context->end() || !triggerKind->is_number())
        return result;

    if (auto val = triggerKind->get<int>(); val >= 1 && val <= 3)
        result.first = (completion_trigger_kind)val;

    if (result.first == completion_trigger_kind::trigger_character)
    {
        auto triggerCharacter = context->find("triggerCharacter");
        if (triggerCharacter != context->end() && triggerCharacter->is_string())
        {
            auto sv = triggerCharacter->get<std::string_view>();
            if (!sv.empty())
                result.second = sv.front();
        }
    }

    return result;
}

// Completion item kinds from the LSP specification
enum class lsp_completion_item_kind
{
    text = 1,
    method = 2,
    function = 3,
    constructor = 4,
    field = 5,
    variable = 6,
    class_v = 7,
    interface = 8,
    module_v = 9,
    property = 10,
    unit = 11,
    value = 12,
    enum_v = 13,
    keyword = 14,
    snippet = 15,
    color = 16,
    file = 17,
    reference = 18,
    folder = 19,
    enum_member = 20,
    constant = 21,
    struct_v = 22,
    event = 23,
    operator_v = 24,
    type_parameter = 25
};


const std::unordered_map<completion_item_kind, lsp_completion_item_kind> completion_item_kind_mapping {
    { completion_item_kind::mach_instr, lsp_completion_item_kind::function },
    { completion_item_kind::asm_instr, lsp_completion_item_kind::function },
    { completion_item_kind::ca_instr, lsp_completion_item_kind::function },
    { completion_item_kind::macro, lsp_completion_item_kind::file },
    { completion_item_kind::var_sym, lsp_completion_item_kind::variable },
    { completion_item_kind::seq_sym, lsp_completion_item_kind::reference },
    { completion_item_kind::ord_sym, lsp_completion_item_kind::field },
};

nlohmann::json get_markup_content(std::string_view content)
{
    return nlohmann::json {
        { "kind", "markdown" },
        { "value", content },
    };
}

std::string decorate_suggestion(std::string_view s)
{
    std::string result;

    for (char c : s)
    {
        if (c < 0x80)
            result.append(2, '#');
        result.append(1, c);
    }
    result.append(2, '#');

    return result;
}

template<typename T>
struct first_arg;
template<typename T, typename Arg, typename... Args>
struct first_arg<T (*)(Arg, Args...)>
{
    using type = Arg;
};
template<typename T, class C, typename Arg, typename... Args>
struct first_arg<T (C::*)(Arg, Args...)>
{
    using type = Arg;
};
template<typename T, class C, typename Arg, typename... Args>
struct first_arg<T (C::*)(Arg, Args...) const>
{
    using type = Arg;
};
template<typename T>
struct first_arg
{
    using type = typename first_arg<decltype(&T::operator())>::type;
};

template<typename U>
auto make_response(const request_id& id, response_provider* response, U handler)
{
    using T = typename first_arg<U>::type;
    class response_t
    {
        request_id m_id;
        response_provider* m_response;
        [[no_unique_address]] U m_handler;

    public:
        response_t(const request_id& id, response_provider* response, U handler)
            : m_id(id)
            , m_response(response)
            , m_handler(std::move(handler))
        {}

        void error(int ec, const char* error) const noexcept
        {
            // terminates on throw
            m_response->respond_error(m_id, "", ec, std::string(error), {});
        }
        void provide(T r) const { m_response->respond(m_id, "", m_handler(std::move(r))); }
    };
    return workspace_manager_response<T>(response_t(id, response, std::move(handler)));
}

} // namespace

feature_language_features::feature_language_features(
    parser_library::workspace_manager& ws_mngr, response_provider& response_provider)
    : feature(response_provider)
    , ws_mngr_(ws_mngr)
{}

void feature_language_features::register_methods(std::map<std::string, method>& methods)
{
    using enum telemetry_log_level;
    const auto add_method = [this, &methods](std::string_view name,
                                auto func,
                                telemetry_log_level telem = telemetry_log_level::NO_TELEMETRY) {
        methods.try_emplace(std::string(name), method { std::bind_front(func, this), telem });
    };

    add_method("textDocument/definition", &feature_language_features::definition, LOG_EVENT);
    add_method("textDocument/references", &feature_language_features::references, LOG_EVENT);
    add_method("textDocument/hover", &feature_language_features::hover, LOG_EVENT);
    add_method("textDocument/completion", &feature_language_features::completion, LOG_EVENT);
    add_method("completionItem/resolve", &feature_language_features::completion_resolve);
    add_method("textDocument/semanticTokens/full", &feature_language_features::semantic_tokens);
    add_method("textDocument/documentSymbol", &feature_language_features::document_symbol);
    add_method("textDocument/$/opcode_suggestion", &feature_language_features::opcode_suggestion);
}

nlohmann::json feature_language_features::register_capabilities()
{
    // in case any changes are done to tokenTypes, the hl_scopes field in protocol.h
    // needs to be adjusted accordingly, as they are implicitly but directly mapped to each other
    return nlohmann::json {
        { "definitionProvider", true },
        { "referencesProvider", true },
        { "hoverProvider", true },
        {
            "completionProvider",
            {
                { "resolveProvider", true },
                { "triggerCharacters", { "&", ".", "_", "$", "#", "@", "*" } },
            },
        },
        {
            "semanticTokensProvider",
            {
                {
                    "legend",
                    {
                        {
                            "tokenTypes",
                            {
                                "class", //         label              = 0
                                "function", //      instruction        = 1
                                "comment", //       remark             = 2
                                "ignored", //       ignored            = 3
                                "comment", //       comment            = 4
                                "macro", //         continuation       = 5
                                "keyword", //       seq_symbol         = 6
                                "variable", //      var_symbol         = 7
                                "operator", //      operator_symbol    = 8
                                "string", //        string             = 9
                                "number", //        number             = 10
                                "parameter", //     operand            = 11
                                "regexp", //        data_def_type      = 12
                                "macro", //         data_def_extension = 13
                                "macro", //         data_attrib_type   = 14
                                "regexp", //        self_def_type      = 15
                                "parameter", //     ordinary_symbol    = 16
                            },
                        },
                        { "tokenModifiers", nlohmann::json::array() },
                    },
                },
                { "full", true },
            },
        },
        { "documentSymbolProvider", true },
    };
}

void feature_language_features::initialize_feature(const nlohmann::json&)
{
    // No need for initialization in this feature.
}

void feature_language_features::definition(const request_id& id, const nlohmann::json& params)
{
    auto document_uri = extract_document_uri(params);
    auto pos = extract_position(params);
    auto resp = make_response(id, response_, [](position_uri definition_position_uri) {
        return nlohmann::json {
            { "uri", definition_position_uri.file_uri() },
            { "range", range_to_json({ definition_position_uri.pos(), definition_position_uri.pos() }) },
        };
    });

    ws_mngr_.definition(document_uri.c_str(), pos, resp);

    response_->register_cancellable_request(id, std::move(resp));
}

void feature_language_features::references(const request_id& id, const nlohmann::json& params)
{
    auto document_uri = extract_document_uri(params);
    auto pos = extract_position(params);

    auto resp = make_response(id, response_, [](position_uri_list references) {
        auto to_ret = nlohmann::json::array();
        for (size_t i = 0; i < references.size(); ++i)
        {
            auto ref = references.item(i);
            to_ret.push_back(
                nlohmann::json { { "uri", ref.file_uri() }, { "range", range_to_json({ ref.pos(), ref.pos() }) } });
        }
        return to_ret;
    });
    ws_mngr_.references(document_uri.c_str(), pos, resp);

    response_->register_cancellable_request(id, std::move(resp));
}
void feature_language_features::hover(const request_id& id, const nlohmann::json& params)
{
    auto document_uri = extract_document_uri(params);
    auto pos = extract_position(params);

    auto resp = make_response(id, response_, [](sequence<char> hover_list_result) {
        std::string_view hover_list(hover_list_result);
        return nlohmann::json {
            { "contents", hover_list.empty() ? "" : get_markup_content(hover_list) },
        };
    });
    ws_mngr_.hover(document_uri.c_str(), pos, resp);

    response_->register_cancellable_request(id, std::move(resp));
}

void feature_language_features::completion(const request_id& id, const nlohmann::json& params)
{
    auto document_uri = extract_document_uri(params);
    auto pos = extract_position(params);

    auto [trigger_kind, trigger_char] = extract_trigger(params);

    auto resp = make_response(
        id, response_, [this](completion_list cl) { return translate_completion_list_and_save_doc(std::move(cl)); });
    ws_mngr_.completion(document_uri.c_str(), pos, trigger_char, trigger_kind, resp);

    response_->register_cancellable_request(id, std::move(resp));
}

nlohmann::json feature_language_features::translate_completion_list_and_save_doc(completion_list list)
{
    auto to_ret = nlohmann::json::object();
    auto completion_item_array = nlohmann::json::array();
    saved_completion_list_doc.clear();
    for (size_t i = 0; i < list.size(); ++i)
    {
        const auto& item = list.item(i);
        auto& json_item = completion_item_array.emplace_back(nlohmann::json {
            { "label", item.label() },
            { "kind", completion_item_kind_mapping.at(item.kind()) },
            { "detail", item.detail() },
            { "insertText", item.insert_text() },
            { "insertTextFormat", 1 + (int)item.is_snippet() },
        });
        saved_completion_list_doc.emplace(item.label(), item.documentation());
        if (auto suggestion = item.suggestion_for(); !suggestion.empty())
        {
            json_item["filterText"] = std::string("~~~") + decorate_suggestion(suggestion);
            json_item["sortText"] = std::string("~~~") + std::string(item.label());
        }
    }
    // needs to be incomplete, otherwise we are unable to include new suggestions
    // when user continues typing (vscode keeps using the first list)
    to_ret["isIncomplete"] = true;
    to_ret["items"] = std::move(completion_item_array);

    return to_ret;
}

void feature_language_features::completion_resolve(const request_id& id, const nlohmann::json& params)
{
    auto response = params;
    if (auto it = response.find("label"); it != response.end() && it->is_string())
    {
        if (auto doc = saved_completion_list_doc.find(it->get<std::string>()); doc != saved_completion_list_doc.end())
            response["documentation"] = get_markup_content(doc->second);
        else
            response["documentation"] = "";
    }
    response_->respond(id, "", std::move(response));
}

void add_token(nlohmann::json& encoded_tokens,
    const parser_library::token_info& current,
    parser_library::range& last_rng,
    bool first)
{
    using namespace parser_library;

    const range& rng = current.token_range;


    size_t delta_line = rng.start.line - last_rng.start.line;

    size_t delta_char =
        last_rng.start.line != rng.start.line ? rng.start.column : rng.start.column - last_rng.start.column;

    size_t length = (rng.start.column > rng.end.column) ? rng.start.column <= 72 ? 72 - rng.start.column : 1
                                                        : rng.end.column - rng.start.column;

    // skip overlaying tokens
    if (delta_line == 0 && delta_char == 0 && !first)
        return;

    encoded_tokens.push_back(delta_line);
    encoded_tokens.push_back(delta_char);
    encoded_tokens.push_back(length);
    encoded_tokens.push_back(static_cast<std::underlying_type_t<semantics::hl_scopes>>(current.scope));
    encoded_tokens.push_back((size_t)0);

    last_rng = current.token_range;
}

nlohmann::json feature_language_features::convert_tokens_to_num_array(
    const std::vector<parser_library::token_info>& tokens)
{
    using namespace parser_library;

    if (tokens.empty())
        return nlohmann::json::array();

    auto encoded_tokens = nlohmann::json::array();


    range last_rng;
    auto current = tokens.cbegin();
    auto next = current + 1;

    // In case of overlapping tokens, we need to remember, that there is a token in the background of currently
    // processed tokens. For example variable symbols inside string '&VAR'.
    // The size of stack will probably not exceed 1 with current grammar.
    std::stack<token_info> to_end;

    while (next != tokens.cend() || !to_end.empty())
    {
        while (!to_end.empty())
        {
            if (last_rng.end < current->token_range.start)
            { // The next token starts before the background token ends, so fill the space between last token and next
              // one with the background scope.
                position curr_end = position::min(to_end.top().token_range.end, current->token_range.start);
                add_token(encoded_tokens, token_info({ last_rng.end, curr_end }, to_end.top().scope), last_rng, false);
            }


            if (to_end.top().token_range.end < current->token_range.end)
                to_end.pop(); // We ran past the background token.
            else
                break;
        }

        if (next == tokens.cend())
            break;


        if (next->token_range.start < current->token_range.end)
        { // The next token starts in the middle of current one.
            add_token(encoded_tokens,
                token_info({ current->token_range.start, next->token_range.start }, current->scope),
                last_rng,
                current == tokens.cbegin());
            to_end.push(*current);
        }
        else
            add_token(encoded_tokens, *current, last_rng, current == tokens.cbegin());

        current = next;
        ++next;
    }

    add_token(encoded_tokens, *current, last_rng, current == tokens.cbegin());
    // End background tokens.
    while (!to_end.empty())
    {
        add_token(encoded_tokens,
            token_info({ last_rng.end, to_end.top().token_range.end }, to_end.top().scope),
            last_rng,
            false);
        to_end.pop();
    }


    return encoded_tokens;
}

void feature_language_features::semantic_tokens(const request_id& id, const nlohmann::json& params)
{
    auto document_uri = extract_document_uri(params);

    auto resp = make_response(id, response_, [](continuous_sequence<token_info> token_list) {
        return nlohmann::json {
            { "data", convert_tokens_to_num_array(std::vector<parser_library::token_info>(std::move(token_list))) },
        };
    });
    ws_mngr_.semantic_tokens(document_uri.c_str(), resp);

    response_->register_cancellable_request(id, std::move(resp));
}

// document symbol item kinds from the LSP specification
enum class lsp_document_symbol_item_kind
{
    File = 1,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    String = 15,
    Number = 16,
    Boolean = 17,
    Array = 18,
    Object = 19,
    Key = 20,
    Null = 21,
    EnumMember = 22,
    Struct = 23,
    Event = 24,
    Operator = 25,
    TypeParameter = 26
};


const std::unordered_map<parser_library::document_symbol_kind, lsp_document_symbol_item_kind>
    document_symbol_item_kind_mapping { { parser_library::document_symbol_kind::DAT,
                                            lsp_document_symbol_item_kind::Array },
        { parser_library::document_symbol_kind::EQU, lsp_document_symbol_item_kind::Boolean },
        { parser_library::document_symbol_kind::MACH, lsp_document_symbol_item_kind::Constant },
        { parser_library::document_symbol_kind::ASM, lsp_document_symbol_item_kind::Boolean },
        { parser_library::document_symbol_kind::UNKNOWN, lsp_document_symbol_item_kind::Operator },
        { parser_library::document_symbol_kind::VAR, lsp_document_symbol_item_kind::Variable },
        { parser_library::document_symbol_kind::SEQ, lsp_document_symbol_item_kind::String },
        { parser_library::document_symbol_kind::COMMON, lsp_document_symbol_item_kind::Struct },
        { parser_library::document_symbol_kind::DUMMY, lsp_document_symbol_item_kind::Class },
        { parser_library::document_symbol_kind::EXECUTABLE, lsp_document_symbol_item_kind::Object },
        { parser_library::document_symbol_kind::READONLY, lsp_document_symbol_item_kind::Enum },
        { parser_library::document_symbol_kind::EXTERNAL, lsp_document_symbol_item_kind::Enum },
        { parser_library::document_symbol_kind::WEAK_EXTERNAL, lsp_document_symbol_item_kind::Enum },
        { parser_library::document_symbol_kind::MACRO, lsp_document_symbol_item_kind::File } };

nlohmann::json feature_language_features::document_symbol_item_json(
    hlasm_plugin::parser_library::document_symbol_item symbol)
{
    return {
        { "name", symbol.name() },
        { "kind", document_symbol_item_kind_mapping.at(symbol.kind()) },
        { "range", range_to_json(symbol.symbol_range()) },
        { "selectionRange", range_to_json(symbol.symbol_selection_range()) },
        { "children", document_symbol_list_json(symbol.children()) },
    };
}

nlohmann::json feature_language_features::document_symbol_list_json(
    hlasm_plugin::parser_library::document_symbol_list symbol_list)
{
    auto result = nlohmann::json::array();
    for (const auto& symbol : symbol_list)
    {
        result.push_back(document_symbol_item_json(symbol));
    }
    return result;
}

void feature_language_features::document_symbol(const request_id& id, const nlohmann::json& params)
{
    auto document_uri = extract_document_uri(params);

    const auto limit = 5000LL;

    auto resp = make_response(
        id, response_, [this](document_symbol_list symbol_list) { return document_symbol_list_json(symbol_list); });

    ws_mngr_.document_symbol(document_uri.c_str(), limit, resp);

    response_->register_cancellable_request(id, std::move(resp));
}

void feature_language_features::opcode_suggestion(const request_id& id, const nlohmann::json& params)
{
    auto document_uri = extract_document_uri(params);

    bool extended = false;
    if (auto e = params.find("extended"); e != params.end() && e->is_boolean())
        extended = e->get<bool>();

    auto opcodes = params.find("opcodes");
    if (opcodes == params.end() || !opcodes->is_array())
    {
        response_->respond(id,
            "",
            nlohmann::json {
                { "uri", document_uri },
                { "suggestions", nlohmann::json::object() },
            });
        return;
    }

    class composite_response_t : public std::enable_shared_from_this<composite_response_t>
    {
        request_id m_id;
        response_provider* m_response;
        std::string m_document_uri;

        nlohmann::json m_suggestions = nlohmann::json::object();

        size_t m_pending_responses = 1;
        bool m_failed = false;

        void send()
        {
            if (m_failed)
                return;

            m_response->respond(m_id,
                "",
                nlohmann::json {
                    { "uri", std::move(m_document_uri) },
                    { "suggestions", std::move(m_suggestions) },
                });
        }

    public:
        composite_response_t(const request_id& id, response_provider* response, std::string document_uri)
            : m_id(id)
            , m_response(response)
            , m_document_uri(std::move(document_uri))
        {}

        void error(int ec, const char* error) noexcept
        {
            m_failed = true;
            // terminates on throw
            m_response->respond_error(m_id, "", ec, std::string(error), {});
        }

        void provide(std::optional<std::pair<std::string, nlohmann::json>> r)
        {
            if (m_failed)
                return;

            if (r.has_value())
                m_suggestions[r->first] = std::move(r->second);

            if (--m_pending_responses == 0)
                send();
        }

        void requests_submitted()
        {
            if (--m_pending_responses == 0)
                send();
        }

        using subrequest_t =
            workspace_manager_response<continuous_sequence<hlasm_plugin::parser_library::opcode_suggestion>>;
        subrequest_t start_request(std::string opcode)
        {
            struct subrequest_t
            {
                std::shared_ptr<composite_response_t> m_self;
                std::string m_opcode;

                void error(int ec, const char* error) const noexcept { m_self->error(ec, error); }

                void provide(continuous_sequence<hlasm_plugin::parser_library::opcode_suggestion> opcode_suggestions)
                {
                    if (opcode_suggestions.size() == 0)
                    {
                        m_self->provide(std::nullopt);
                        return;
                    }
                    auto result = nlohmann::json::array();
                    for (const auto& s : opcode_suggestions)
                    {
                        result.push_back(nlohmann::json {
                            { "opcode", std::string_view(s.opcode.data(), s.opcode.size()) },
                            { "distance", s.distance },
                        });
                    }
                    m_self->provide(std::make_pair(std::move(m_opcode), std::move(result)));
                }
            };

            auto [result, _] = make_workspace_manager_response(subrequest_t { shared_from_this(), std::move(opcode) });

            ++m_pending_responses;

            return std::move(result);
        }

        auto get_invalidator(std::vector<composite_response_t::subrequest_t> collected_subrequests)
        {
            struct
            {
                std::shared_ptr<composite_response_t> m_self;
                std::vector<composite_response_t::subrequest_t> subrequests;

                bool resolved() const { return m_self->m_failed || m_self->m_pending_responses == 0; }
                void invalidate() const
                {
                    for (const auto& subreq : subrequests)
                        subreq.invalidate();
                }
            } result { shared_from_this(), std::move(collected_subrequests) };
            return result;
        }
    };

    auto composite = std::make_shared<composite_response_t>(id, response_, document_uri);

    try
    {
        std::vector<composite_response_t::subrequest_t> subrequests;
        for (const auto& opcode : *opcodes)
        {
            if (!opcode.is_string())
                continue;
            auto op = opcode.get<std::string>();

            ws_mngr_.make_opcode_suggestion(
                document_uri.c_str(), op.c_str(), extended, subrequests.emplace_back(composite->start_request(op)));
        }

        response_->register_cancellable_request(id, composite->get_invalidator(std::move(subrequests)));

        composite->requests_submitted();
    }
    catch (const std::exception& e)
    {
        composite->error(utils::error::lsp::internal_error, e.what());
    }
}

} // namespace hlasm_plugin::language_server::lsp
