#include <chrono>
#include <ctime>
#include <iostream>
#include "json.hpp"
#include <fstream>
#include "shared/workspace_manager.h"
#include "../parser_library/src/file_impl.h"

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
	virtual void consume_performance_metrics(const hlasm_plugin::parser_library::performance_metrics & metrics) override
	{
		metrics_ = metrics;
	}

	hlasm_plugin::parser_library::performance_metrics metrics_;
};

int main(int argc, char** argv)
{
	std::string ws_folder;
	// no arguments
	if (argc < 2)
		ws_folder = ".";
	else
		// configuration path
		ws_folder = argv[1];

	auto conf_path = ws_folder + "/pgm_conf.json";

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

		for (auto program : programs)
		{
			// program file
			auto source_file = program["program"].get<std::string>();
			auto source_path = ws_folder + "/" + source_file;
			in = std::ifstream(source_path);
			if (in.fail())
				continue;
			program_count++;
			// program's contents
			auto content = std::string((std::istreambuf_iterator<char>(in)), (std::istreambuf_iterator<char>()));
			content = hlasm_plugin::parser_library::file_impl::replace_non_utf8_chars(content);

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
				<< "Executed Statement/ms: " << exec_statements /(double)time << '\n'
				<< "Line/ms: " << collector.metrics_.lines/(double)time << '\n'
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

		}

		std::clog << "Programs: " << program_count << '\n'
					<< "Benchmarked files: " <<  all_files << '\n'
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