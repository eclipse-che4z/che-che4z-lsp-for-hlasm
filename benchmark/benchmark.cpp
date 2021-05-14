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

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_map>

#include "config/pgm_conf.h"
#include "nlohmann/json.hpp"
#include "workspace_manager.h"
#include "workspaces/file_impl.h"

/*
 * The benchmark is used to evaluate multiple aspects about the performance and accuracy of the parse library.
 * To collect the metrics, a performance_metrics_consumer observer must be implemented and registered within the parsing
 *library. The user specifies a standard HLASM workspace folder and the benchmark calls did_open_file for each program
 *file defined in the workspace's pgm_conf.json. After each parsed files, it retrieves the performance metrics and
 *outputs them immediately to the console. After the whole benchmark is done, a json with the outputs is created.
 *
 * Accepted parameters:
 *	-r - range of files to be parsed in form start-end. By default, all defined files are parsed.
 *  -c - single file to be parsed over and over again
 *  -p - path to the folder with .hlasmplugin
 * Collected metrics:
 * - Errors                   - number of errors encountered during the parsing
 * - Warnings                 - number of warnings encountered during the parsing
 * - Wall Time                - parsing duration, wall time
 * - CPU Time                 - parsing duration, CPU time
 * - Open Code Statements     - number of statements parsed in open code
 * - Copy Statements          - number of statements parsed in copy files (includes multiple uses of the same copy file)
 * - Macro Statements         - number of statements parsed in macro files (includes multiple uses of the same macro)
 * - Copy Def Statements      - number of statements defined in copy files (only the first occurence of the copy file)
 * - Macro Def Statements     - number of statements defined in macro files (only the first occurence of the macro)
 * - Lookahead Statements     - number of statements processed in lookahead mode
 * - Reparsed Statements      - number of statements that were reparsed later (e.g. model statements)
 * - Continued Statements     - number of statements that were continued (multiple continuations of one statement count
 *as one continued statement)
 * - Non-continued Statements - number of statements that were not continued
 * - Lines                    - total number of lines
 * - ExecStatement/ms         - ExecStatements includes open code, macro, copy, lookahead and reparsed statements
 * - Line/ms
 * - Files                    - total number of parsed files
 */

using json = nlohmann::json;

class diagnostic_counter : public hlasm_plugin::parser_library::diagnostics_consumer
{
public:
    void consume_diagnostics(hlasm_plugin::parser_library::diagnostic_list diagnostics) override
    {
        for (size_t i = 0; i < diagnostics.diagnostics_size(); i++)
        {
            auto diag_sev = diagnostics.diagnostics(i).severity();
            if (diag_sev == hlasm_plugin::parser_library::diagnostic_severity::error)
                error_count++;
            else if (diag_sev == hlasm_plugin::parser_library::diagnostic_severity::warning)
                warning_count++;
            message_counts[diagnostics.diagnostics(i).code()]++;
        }
    }

    void clear_counters()
    {
        error_count = 0;
        warning_count = 0;
        message_counts.clear();
    }

    size_t error_count = 0;
    size_t warning_count = 0;

    std::unordered_map<std::string, unsigned> message_counts;
};

class metrics_collector : public hlasm_plugin::parser_library::performance_metrics_consumer
{
public:
    void consume_performance_metrics(const hlasm_plugin::parser_library::performance_metrics& metrics) override
    {
        metrics_ = metrics;
    }

    hlasm_plugin::parser_library::performance_metrics metrics_;
};

struct all_file_stats
{
    double average_line_ms = 0;
    double average_stmt_ms = 0;
    size_t all_files = 0;
    long long whole_time = 0;
    size_t program_count = 0;
    size_t parsing_crashes = 0;
    size_t reparsing_crashes = 0;
    size_t failed_file_opens = 0;
};

json get_top_messages(const std::unordered_map<std::string, unsigned>& msgs, size_t limit = 3)
{
    std::vector<std::pair<std::string, unsigned>> top_msgs(limit);

    constexpr const auto cmp_msg = [](const auto& a, const auto& b) { return a.second > b.second; };

    const auto last = std::partial_sort_copy(msgs.begin(), msgs.end(), top_msgs.begin(), top_msgs.end(), cmp_msg);
    top_msgs.erase(last, top_msgs.end());

    json result = json::object();
    for (auto&& [key, value] : top_msgs)
        result[std::move(key)] = value;
    return result;
}

json parse_one_file(const std::string& source_file,
    const std::string& ws_folder,
    all_file_stats& s,
    bool write_details,
    const std::string& message,
    bool do_reparse)
{
    auto source_path = ws_folder + "/" + source_file;
    std::ifstream in(source_path);
    if (in.fail())
    {
        ++s.failed_file_opens;
        std::clog << "File read error: " << source_path << std::endl;
        return json({ { "File", source_file }, { "Success", false }, { "Reason", "Read error" } });
    }
    s.program_count++;
    // program's contents
    auto content = std::string((std::istreambuf_iterator<char>(in)), (std::istreambuf_iterator<char>()));
    content = hlasm_plugin::parser_library::workspaces::file_impl::replace_non_utf8_chars(content);

    // new workspace manager
    hlasm_plugin::parser_library::workspace_manager ws;
    diagnostic_counter diag_counter;
    ws.register_diagnostics_consumer(&diag_counter);
    metrics_collector collector;
    ws.register_performance_metrics_consumer(&collector);
    // input folder as new workspace
    ws.add_workspace(ws_folder.c_str(), ws_folder.c_str());

    // start counting
    auto c_start = std::clock();
    auto start = std::chrono::high_resolution_clock::now();
    std::clog << message << "Parsing file: " << source_file << std::endl;
    // open file/parse
    try
    {
        ws.did_open_file(source_path.c_str(), 1, content.c_str(), content.length());
    }
    catch (const std::exception& e)
    {
        ++s.parsing_crashes;
        std::clog << message << "Error: " << e.what() << std::endl;
        return json({ { "File", source_file }, { "Success", false }, { "Reason", "Crash" } });
    }
    catch (...)
    {
        ++s.parsing_crashes;
        std::clog << message << "Parse failed\n\n" << std::endl;
        return json({ { "File", source_file }, { "Success", false }, { "Reason", "Crash" } });
    }

    auto c_end = std::clock();
    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto exec_statements = collector.metrics_.open_code_statements + collector.metrics_.copy_statements
        + collector.metrics_.macro_statements + collector.metrics_.lookahead_statements
        + collector.metrics_.reparsed_statements;
    s.average_stmt_ms += (exec_statements / (double)time);
    s.average_line_ms += collector.metrics_.lines / (double)time;
    s.all_files += collector.metrics_.files;
    s.whole_time += time;

    auto top_messages = get_top_messages(diag_counter.message_counts);

    json result({ { "File", source_file },
        { "Success", true },
        { "Errors", diag_counter.error_count },
        { "Warnings", diag_counter.warning_count },
        { "Wall Time (ms)", time },
        { "CPU Time (ms/n)", 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC },
        { "Open Code Statements", collector.metrics_.open_code_statements },
        { "Copy Statements", collector.metrics_.copy_statements },
        { "Macro Statements", collector.metrics_.macro_statements },
        { "Copy Def Statements", collector.metrics_.copy_def_statements },
        { "Macro Def Statements", collector.metrics_.macro_def_statements },
        { "Lookahead Statements", collector.metrics_.lookahead_statements },
        { "Reparsed Statements", collector.metrics_.reparsed_statements },
        { "Continued Statements", collector.metrics_.continued_statements },
        { "Non-continued Statements", collector.metrics_.non_continued_statements },
        { "Executed Statements", exec_statements },
        { "Lines", collector.metrics_.lines },
        { "ExecStatement/ms", exec_statements / (double)time },
        { "Line/ms", collector.metrics_.lines / (double)time },
        { "Files", collector.metrics_.files },
        { "Top messages", std::move(top_messages) } });

    auto first_parse_metrics = collector.metrics_;
    auto first_diag_counter = diag_counter;
    long long reparse_time = 0;
    // Reparse to benchmark macro caching
    if (do_reparse)
    {
        diag_counter.clear_counters();

        auto c_start_reparse = std::clock();
        auto start_reparse = std::chrono::high_resolution_clock::now();
        std::clog << message << "Reparsing file: " << source_file << std::endl;
        // open file/parse
        try
        {
            ws.did_change_file(source_path.c_str(), 1, nullptr, 0);
        }
        catch (const std::exception& e)
        {
            ++s.reparsing_crashes;
            std::clog << message << "Reparse error: " << e.what() << std::endl;
            return json({ { "File", source_file }, { "Success", false }, { "Reason", "Crash" }, { "Reparse", true } });
        }
        catch (...)
        {
            ++s.reparsing_crashes;
            std::clog << message << "Reparse failed\n\n" << std::endl;
            return json({ { "File", source_file }, { "Success", false }, { "Reason", "Crash" }, { "Reparse", true } });
        }

        auto c_end_reparse = std::clock();
        reparse_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start_reparse)
                           .count();
        result["Reparse Wall Time (ms)"] = reparse_time;
        result["Reparse CPU Time (ms/n)"] = 1000.0 * (c_end_reparse - c_start_reparse) / CLOCKS_PER_SEC;
        result["Reparse errors"] = diag_counter.error_count;
        result["Reparse warnings"] = diag_counter.warning_count;
    }

    if (write_details)
        std::clog << "Time: " << time << " ms" << '\n'
                  << "Reparse time: " << reparse_time << " ms" << '\n'
                  << "Errors: " << first_diag_counter.error_count << '\n'
                  << "Reparse errors: " << diag_counter.error_count << '\n'
                  << "Open Code Statements: " << first_parse_metrics.open_code_statements << '\n'
                  << "Copy Statements: " << first_parse_metrics.copy_statements << '\n'
                  << "Macro Statements: " << first_parse_metrics.macro_statements << '\n'
                  << "Copy Def Statements: " << first_parse_metrics.copy_def_statements << '\n'
                  << "Macro Def Statements: " << first_parse_metrics.macro_def_statements << '\n'
                  << "Lookahead Statements: " << first_parse_metrics.lookahead_statements << '\n'
                  << "Reparsed Statements: " << first_parse_metrics.reparsed_statements << '\n'
                  << "Continued Statements: " << first_parse_metrics.continued_statements << '\n'
                  << "Non-continued Statements: " << first_parse_metrics.non_continued_statements << '\n'
                  << "Lines: " << first_parse_metrics.lines << '\n'
                  << "Executed Statement/ms: " << exec_statements / (double)time << '\n'
                  << "Line/ms: " << first_parse_metrics.lines / (double)time << '\n'
                  << "Files: " << first_parse_metrics.files << '\n'
                  << "Top messages: " << top_messages.dump() << '\n'
                  << '\n'
                  << std::endl;

    return result;
}

std::string get_file_message(size_t iter, size_t begin, size_t end, const std::string& base_message)
{
    if (base_message == "")
        return "";
    std::stringstream s;
    s << "[" << base_message << " " << iter << "/(" << begin << "-" << end << ")] ";
    return s.str();
}

int main(int argc, char** argv)
{
    std::string ws_folder = std::filesystem::current_path().string();
    std::string single_file = "";
    size_t start_range = 0, end_range = 0;
    bool write_details = true;
    bool do_reparse = true;
    std::string message;
    for (int i = 1; i < argc - 1; i++)
    {
        std::string arg = argv[i];
        // range parameter, format start-end
        if (arg == "-r")
        {
            std::string val = argv[i + 1];
            auto pos = val.find('-');
            if (pos == val.npos)
            {
                std::clog << "Range parameter should be in format Start-End" << '\n';
                return 1;
            }
            try
            {
                start_range = std::stoi(val.substr(0, pos));
                end_range = std::stoi(val.substr(pos + 1));
            }
            catch (...)
            {
                std::clog << "Range values must be integers" << '\n';
                return 1;
            }
            i++;
        }
        // path parameter, path to the folder containing .hlasmplugin
        else if (arg == "-p")
        {
            ws_folder = argv[i + 1];
            i++;
        }
        // cycle parameter, loop infinitely single file
        else if (arg == "-c")
        {
            single_file = argv[i + 1];
            i++;
        }
        // details switch, when specified, details are not outputted to stderr
        else if (arg == "-d")
        {
            write_details = false;
        }
        // When specified, skip reparsing each program to test out macro caching
        else if (arg == "-r")
            do_reparse = false;
        // When specified, the scpecified string will be shown at the beginning of each "Parsing <file>" message
        else if (arg == "-m")
        {
            message = argv[i + 1];
            i++;
        }
        else
        {
            std::clog << "Unknown parameter " << arg << '\n';
            return 1;
        }
    }

    auto conf_path = ws_folder + "/.hlasmplugin/pgm_conf.json";

    std::ifstream in(conf_path);
    if (in.fail())
    {
        std::clog << "Non existing config: " << conf_path << '\n';
        return 1;
    }

    // configuration contents
    std::string conf((std::istreambuf_iterator<char>(in)), (std::istreambuf_iterator<char>()));

    hlasm_plugin::parser_library::config::pgm_conf program_config;
    try
    {
        // parse pgm_conf.json
        json::parse(conf).get_to(program_config);
    }
    catch (...)
    {
        std::clog << "Malformed json" << std::endl;
        return 1;
    }

    all_file_stats s;
    if (single_file != "")
    {
        if (end_range == 0)
            end_range = std::numeric_limits<long long int>::max();
        for (size_t i = 0; i < end_range; ++i)
        {
            json j = parse_one_file(single_file,
                ws_folder,
                s,
                write_details,
                get_file_message(i, start_range, end_range, message),
                do_reparse);
            std::cout << j.dump(2);
            std::cout.flush();
        }
    }
    else
    {
        std::cout << "{\n\"pgms\" : [";
        std::cout.flush();
        size_t current_iter = 0;
        bool not_first = false;
        for (const auto& pgm : program_config.pgms)
        {
            if (current_iter >= end_range && end_range > 0)
                break;
            if (current_iter < start_range)
            {
                current_iter++;
                continue;
            }
            const std::string& source_file = pgm.program;

            json j = parse_one_file(source_file,
                ws_folder,
                s,
                write_details,
                get_file_message(current_iter, start_range, end_range, message),
                do_reparse);

            if (not_first)
                std::cout << ",\n";
            else
                not_first = true;
            std::cout << j.dump(2);
            std::cout.flush();
            current_iter++;
        }
        std::cout << "],\n\"total\" : ";

        std::clog << "Programs: " << s.program_count << '\n'
                  << "Benchmarked files: " << s.all_files << '\n'
                  << "Analyzer crashes: " << s.parsing_crashes << '\n'
                  << "Failed program opens: " << s.failed_file_opens << '\n'
                  << "Benchmark time: " << s.whole_time << " ms" << '\n'
                  << "Average statement/ms: " << s.average_stmt_ms / (double)program_config.pgms.size() << '\n'
                  << "Average line/ms: " << s.average_line_ms / (double)program_config.pgms.size() << "\n\n"
                  << std::endl;

        std::cout << json({ { "Programs", s.program_count },
                              { "Benchmarked files", s.all_files },
                              { "Benchmark time(ms)", s.whole_time },
                              { "Analyzer crashes", s.parsing_crashes },
                              { "Failed program opens", s.failed_file_opens },
                              { "Average statement/ms", s.average_stmt_ms / (double)program_config.pgms.size() },
                              { "Average line/ms", s.average_line_ms / (double)program_config.pgms.size() } })
                         .dump(2);
        std::cout << "}\n";
        std::clog << "Parse finished\n\n" << std::endl;
    }
    return 0;
}
