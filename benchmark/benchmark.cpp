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
	virtual void consume_diagnostics(hlasm_plugin::parser_library::diagnostic_list diagnostics)
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
		std::clog << "Non existing config: " << conf_path << std::endl;
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
		for (auto program : programs)
		{
			// program file
			auto source_file = program["program"].get<std::string>();
			auto source_path = ws_folder + "/" + source_file;
			in = std::ifstream(source_path);
			if (in.fail())
				continue;
			// program's contents
			auto content = std::string((std::istreambuf_iterator<char>(in)), (std::istreambuf_iterator<char>()));
			content = hlasm_plugin::parser_library::file_impl::replace_non_utf8_chars(content);

			// new workspace manager
			hlasm_plugin::parser_library::workspace_manager ws;
			diagnostic_counter consumer;
			ws.register_diagnostics_consumer(&consumer);
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
				std::clog << "Parse failed" << std::endl;
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

			std::clog << "Parse succeeded after "
				<< std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
				<< "ms with "
				<< consumer.error_count
				<< " errors" << std::endl;

			result.push_back(json(
				{
					{"File", source_file },
					{"Success", true},
					{"Errors", consumer.error_count},
					{"Warnings", consumer.warning_count },
					{"Wall Time (ms)", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()},
					{"CPU Time (ms/n)", 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC}
				}
			));
		}

		std::clog << "Parse finished" << std::endl;

		std::cout << result << std::endl;
	}
	catch (...)
	{
		std::clog << "Malformed json" << std::endl;
		return 1;
	}


	return 0;
}