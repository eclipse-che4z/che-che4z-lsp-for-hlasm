#include <string>
#include <filesystem>
#include <string>
#include <regex>

#include "workspace.h"
#include "json.hpp"

namespace hlasm_plugin {
namespace parser_library {

constexpr char FILENAME_PROC_GRPS[] = "proc_grps.json";
constexpr char FILENAME_PGM_CONF[] = "pgm_conf.json";


hlasm_plugin::parser_library::workspace::workspace(ws_uri uri, std::string name, file_manager & file_manager)
	: name_(name), uri_(uri), file_manager_(file_manager), implicit_proc_grp("pg_implicit")
{
}

hlasm_plugin::parser_library::workspace::workspace(ws_uri uri, file_manager & file_manager) : workspace(uri, uri, file_manager)
{
}

bool program_id_match(const std::string & filename, const program_id & program)
{
	//TODO: actual wildcard matching instead of regex
	std::regex prg_regex(program);
	return std::regex_match(filename, prg_regex);
}

const processor_group & workspace::get_proc_grp_by_program(const std::string & filename) const
{
	assert(opened_);
	for (const auto & pgm : pgm_conf_)
	{
		std::filesystem::path fname_path(filename);
		
		if(program_id_match(fname_path.lexically_relative(uri_).lexically_normal().string(), pgm.program_id))
			return proc_grps_.at(pgm.pgroup);
	}
	return implicit_proc_grp;
}

void hlasm_plugin::parser_library::workspace::open()
{
	if (opened_)
		return;
	load_config();
	opened_ = true;
}

void hlasm_plugin::parser_library::workspace::close()
{
	opened_ = false;
}

const processor_group & workspace::get_proc_grp(const proc_grp_id & proc_grp) const
{
	assert(opened_);
	return proc_grps_.at(proc_grp);
}

//open config files and parse them
void hlasm_plugin::parser_library::workspace::load_config()
{
	std::filesystem::path ws_path(uri_);
	using json = nlohmann::json;

	//TODO diagnostics: check if pgm_conf refers to existing P2, otherwise error
	//TODO diagnostics: parse error propagation(probably in caller function?)

	//proc_grps.json parse
	file & proc_grps_file = file_manager_.add((ws_path / FILENAME_PROC_GRPS).string());
	files_.emplace(proc_grps_file.get_file_name(), &proc_grps_file);

	json proc_grps_json = nlohmann::json::parse(proc_grps_file.get_text());

	json pgs = proc_grps_json["pgroups"];

	for (auto & pg : pgs)
	{
		const std::string & name = pg["name"].get<std::string>();
		const json & libs = pg["libs"];

		processor_group prc_grp(name);

		for (auto & lib_path_json : libs)
		{
			if (lib_path_json.is_string())
			{
				//the added '/' will ensure the path always ends with directory separator
				const std::string p = lib_path_json.get<std::string>();
				std::filesystem::path lib_path(p.empty() ? p : p + '/');
				if(lib_path.is_absolute())
					prc_grp.libs.push_back(std::make_unique<library_local>(file_manager_, lib_path.lexically_normal().string()));
				else if (lib_path.is_relative())
					prc_grp.libs.push_back(std::make_unique<library_local>(file_manager_, (ws_path / lib_path).lexically_normal().string()));
				//else ignore, publish warning
			}
		}

		proc_grps_.emplace(std::move(name), std::move(prc_grp));
	}


	//pgm_conf.json parse
	file & pgm_conf_file = file_manager_.add((ws_path / FILENAME_PGM_CONF).string());
	files_.emplace(pgm_conf_file.get_file_name(), &pgm_conf_file);

	json pgm_conf_json = nlohmann::json::parse(pgm_conf_file.get_text());

	json pgms = pgm_conf_json["pgms"];

	for (auto & pgm : pgms)
	{
		pgm_conf_.emplace_back(pgm["program"].get<std::string>(), pgm["pgroup"].get<std::string>() );
	}
}


}
}
