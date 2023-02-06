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

#include "wildcard.h"

#include "utils/platform.h"

namespace hlasm_plugin::parser_library::workspaces {
namespace {
// used for wildcard to regex conversions
const std::regex escape("(\\(|\\[|\\{|\\\\|\\^|\\-|\\=|\\$|\\!|\\||\\]|\\}|\\)|\\.)");
const std::regex question("\\?");
const std::regex nongreedy("(\\*|\\+)");
const std::regex slash("\\\\");

const std::string single_url_char_matcher = []() {
    const std::string utf_8_continuation_matcher = "(?:%[89AB][0-9A-F])";
    const std::string utf_8_1_byte_matcher = "%[0-7][0-9A-F]";
    const std::string utf_8_2_byte_matcher = "%(?:(?:C[2-9A-F])|(?:D[0-9A-F]))" + utf_8_continuation_matcher;

    const std::string utf_8_3_byte_pattern_overlongs = "0%[AB]";
    const std::string utf_8_3_byte_pattern_straight = "[1-9A-CF]" + utf_8_continuation_matcher;
    const std::string utf_8_3_byte_pattern_surrogates = "D%[89]";
    const std::string utf_8_3_byte_matcher = "%E(?:" + utf_8_3_byte_pattern_straight
        + "|(?:" + utf_8_3_byte_pattern_overlongs + "|" + utf_8_3_byte_pattern_surrogates + ")[0-9A-F])"
        + utf_8_continuation_matcher;

    const std::string utf_8_4_byte_pattern_planes_1_3 = "0%[9AB]";
    const std::string utf_8_4_byte_pattern_planes_4_15 = "[1-3]" + utf_8_continuation_matcher;
    const std::string utf_8_4_byte_pattern_plane_16 = "4%8";
    const std::string utf_8_4_byte_matcher = "%F(?:" + utf_8_4_byte_pattern_planes_4_15
        + "|(?:" + utf_8_4_byte_pattern_planes_1_3 + "|" + utf_8_4_byte_pattern_plane_16 + ")[0-9A-F])"
        + utf_8_continuation_matcher + "{2}";

    const std::string utf_8_char_matcher =
        utf_8_4_byte_matcher + "|" + utf_8_3_byte_matcher + "|" + utf_8_2_byte_matcher + "|" + utf_8_1_byte_matcher;

    return "(?:[^%//]|" + utf_8_char_matcher + ")";
}();
} // namespace

std::regex wildcard2regex(std::string wildcard)
{
    // change of double backslash to forward slash
    wildcard = std::regex_replace(wildcard, slash, "/");
    wildcard = std::regex_replace(wildcard, escape, "\\$1");
    wildcard = std::regex_replace(wildcard, question, single_url_char_matcher);
    wildcard = std::regex_replace(wildcard, nongreedy, ".$1?");

    return std::regex(wildcard);
}

std::regex percent_encoded_pathmask_to_regex(std::string_view s)
{
    std::string r;
    r.reserve(s.size());

    bool path_started = false;
    while (!s.empty())
    {
        switch (auto c = s.front())
        {
            case '*':
                if (s.starts_with("**/"))
                {
                    if (path_started)
                    {
                        path_started = false;
                        r.append("[^/]*[/]");
                    }
                    r.append("(?:.*/)?");
                    s.remove_prefix(3);
                }
                else if (s.starts_with("**"))
                {
                    r.append(".*");
                    s.remove_prefix(2);
                }
                else if (s.starts_with("*/"))
                {
                    path_started = false;
                    r.append("[^/]*[/]");
                    s.remove_prefix(2);
                }
                else
                {
                    r.append("[^/]*");
                    s.remove_prefix(1);
                }
                break;

            case '/':
                path_started = false;
                r.append("[/]");
                s.remove_prefix(1);
                break;

            case '?':
                path_started = true;
                r.append(single_url_char_matcher);
                s.remove_prefix(1);
                break;
            case '^':
            case '$':
            case '+':
            case '.':
            case '(':
            case ')':
            case '|':
            case '{':
            case '}':
            case '[':
            case ']':
                r.push_back('\\');
                [[fallthrough]];
            default:
                path_started = true;
                r.push_back(c);
                s.remove_prefix(1);
                break;
        }
    }

    return std::regex(r);
}

} // namespace hlasm_plugin::parser_library::workspaces
