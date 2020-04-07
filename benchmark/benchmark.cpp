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

#include <chrono>
#include <ctime>
#include <iostream>
#include "json.hpp"
#include <fstream>
#include <filesystem>
#include "workspace_manager.h"
#include "workspaces/file_impl.h"

/*
 * The benchmark is used to evaluate multiple aspects about the performance and accuracy of the parse library.
 * To collect the metrics, a performance_metrics_consumer observer must be implemented and registered within the parsing library.
 * The user specifies a standard HLASM workspace folder and the benchmark calls did_open_file for each program file defined in the workspace's pgm_conf.json.
 * After each parsed files, it retrieves the performance metrics and outputs them immediately to the console. After the whole benchmark is done, a json with the outputs is created.
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
 * - Continued Statements     - number of statements that were continued (multiple continuations of one statement count as one continued statement)
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
	virtual void consume_diagnostics(hlasm_plugin::parser_library::diagnostic_list diagnostics) override
	{
		for (size_t i = 0; i < diagnostics.diagnostics_size(); i++)
		{
			auto diag_sev = diagnostics.diagnostics(i).severity();
			if (diag_sev == hlasm_plugin::parser_library::diagnostic_severity::error)
				error_count++;
			else if (diag_sev == hlasm_plugin::parser_library::diagnostic_severity::warning)
				warning_count++;
		}
	}

	size_t error_count = 0;
	size_t warning_count = 0;
};

class metrics_collector : public hlasm_plugin::parser_library::performance_metrics_consumer
{
public:
	virtual void consume_performance_metrics(const hlasm_plugin::parser_library::performance_metrics& metrics) override
	{
		metrics_ = metrics;
	}

	hlasm_plugin::parser_library::performance_metrics metrics_;
};

int main(int argc, char** argv)
{
	std::string ws_folder = std::filesystem::current_path().string();
	std::string single_file = "";
	size_t start_range = 0, end_range = 0;
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

	try
	{
		// parsed to json
		json conf_json = json::parse(conf);
		// list of programs
		std::vector<json> programs = conf_json["pgms"];
		// results
		json result = json::array();
		// final results
		double average_line_ms = 0;
		double average_stmt_ms = 0;
		size_t all_files = 0;
		long long whole_time = 0;
		size_t program_count = 0;
		size_t current_iter = 0;
		for (auto program : programs)
		{
			if (current_iter < start_range)
			{
				current_iter++;
				continue;
			}
			// program file
			auto source_file = program["program"].get<std::string>();
			if (single_file != "" && source_file != single_file)
				continue;
			while (true)
			{
				auto source_path = ws_folder + "/" + source_file;
				in = std::ifstream(source_path);
				if (in.fail())
					continue;
				program_count++;
				// program's contents
				auto content = std::string((std::istreambuf_iterator<char>(in)), (std::istreambuf_iterator<char>()));
				content = hlasm_plugin::parser_library::workspaces::file_impl::replace_non_utf8_chars(content);

				// new workspace manager
				hlasm_plugin::parser_library::workspace_manager ws;
				diagnostic_counter consumer;
				ws.register_diagnostics_consumer(&consumer);
				metrics_collector collector;
				ws.register_performance_metrics_consumer(&collector);
				// input folder as new workspace
				ws.add_workspace(ws_folder.c_str(), ws_folder.c_str());

				// start counting
				auto c_start = std::clock();
				auto start = std::chrono::high_resolution_clock::now();
				std::clog << "Parsing file: " << source_file << std::endl;
				// open file/parse
				try
				{
					ws.did_open_file(source_path.c_str(), 1, content.c_str(), content.length());
				}
				catch (...)
				{
					std::clog << "Parse failed\n\n" << std::endl;
					result.push_back(json(
						{
							{"File", source_file },
							{"Success", false}
						}
					));
					continue;
				}
				auto c_end = std::clock();
				auto end = std::chrono::high_resolution_clock::now();
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
				auto exec_statements = collector.metrics_.open_code_statements + collector.metrics_.copy_statements + collector.metrics_.macro_statements
					+ collector.metrics_.lookahead_statements + collector.metrics_.reparsed_statements;
				average_stmt_ms += (exec_statements / (double)time);
				average_line_ms += collector.metrics_.lines / (double)time;
				all_files += collector.metrics_.files;
				whole_time += time;

				std::clog << "Time: " << time << " ms" << '\n'
					<< "Errors: " << consumer.error_count << '\n'
					<< "Open Code Statements: " << collector.metrics_.open_code_statements << '\n'
					<< "Copy Statements: " << collector.metrics_.copy_statements << '\n'
					<< "Macro Statements: " << collector.metrics_.macro_statements << '\n'
					<< "Copy Def Statements: " << collector.metrics_.copy_def_statements << '\n'
					<< "Macro Def Statements: " << collector.metrics_.macro_def_statements << '\n'
					<< "Lookahead Statements: " << collector.metrics_.lookahead_statements << '\n'
					<< "Reparsed Statements: " << collector.metrics_.reparsed_statements << '\n'
					<< "Continued Statements: " << collector.metrics_.continued_statements << '\n'
					<< "Non-continued Statements: " << collector.metrics_.non_continued_statements << '\n'
					<< "Lines: " << collector.metrics_.lines << '\n'
					<< "Executed Statement/ms: " << exec_statements / (double)time << '\n'
					<< "Line/ms: " << collector.metrics_.lines / (double)time << '\n'
					<< "Files: " << collector.metrics_.files << "\n\n" << std::endl;

				result.push_back(json(
					{
						{"File", source_file },
						{"Success", true},
						{"Errors", consumer.error_count},
						{"Warnings", consumer.warning_count },
						{"Wall Time (ms)", time},
						{"CPU Time (ms/n)", 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC},
						{"Open Code Statements: " ,collector.metrics_.open_code_statements},
						{"Copy Statements: " ,collector.metrics_.copy_statements},
						{"Macro Statements: " , collector.metrics_.macro_statements},
						{"Copy Def Statements: " , collector.metrics_.copy_def_statements},
						{"Macro Def Statements: " , collector.metrics_.macro_def_statements },
						{"Lookahead Statements: " ,collector.metrics_.lookahead_statements},
						{"Reparsed Statements: " , collector.metrics_.reparsed_statements},
						{"Continued Statements: ",collector.metrics_.continued_statements},
						{"Non-continued Statements: ",collector.metrics_.non_continued_statements},
						{"Lines", collector.metrics_.lines},
						{"ExecStatement/ms", exec_statements / (double)time},
						{"Line/ms", collector.metrics_.lines / (double)time},
						{"Files", collector.metrics_.files}
					}
				));
				if (single_file == "")
					break;
			}
			if (current_iter >= end_range && end_range > 0)
				break;
			current_iter++;
		}

		std::clog << "Programs: " << program_count << '\n'
			<< "Benchmarked files: " << all_files << '\n'
			<< "Benchmark time: " << whole_time << " ms" << '\n'
			<< "Average statement/ms: " << average_stmt_ms / (double)programs.size() << '\n'
			<< "Average line/ms: " << average_line_ms / (double)programs.size() << "\n\n" << std::endl;

		result.push_back(json(
			{
				{"Benchmarked files", all_files},
				{"Benchmark time(ms)", whole_time},
				{"Average statement/ms", average_stmt_ms / (double)programs.size() },
				{"Average line/ms", average_line_ms / (double)programs.size()}
			}
		));

		std::clog << "Parse finished\n\n" << std::endl;

		std::cout << result << std::endl;
	}
	catch (...)
	{
		std::clog << "Malformed json" << std::endl;
		return 1;
	}


	return 0;
}