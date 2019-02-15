#include <string>
#include <filesystem>
#include <string>
#include <regex>

#include "workspace.h"
#include "json.hpp"
#include "parser_tools.h"
#include "processor.h"

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

void workspace::collect_diags() const
{
	for (auto & pg : proc_grps_)
	{
		collect_diags_from_child(pg.second);
	}
}

void workspace::add_proc_grp(processor_group pg)
{
	proc_grps_.emplace(pg.name(), std::move(pg));
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
		
		if(program_id_match(fname_path.lexically_relative(uri_).lexically_normal().string(), pgm.prog_id))
			return proc_grps_.at(pgm.pgroup);
	}
	return implicit_proc_grp;
}



void workspace::did_open_file(const std::string & file_uri)
{
	//add support for hlasm to vscode (auto detection??) and do the decision based on languageid
	if (file_uri == uri_ + "proc_grps.json" || file_uri == uri_ + "pgm_conf.json")
	{
		load_config();
		return;
	}
	
	auto f = file_manager_.find_processor_file(file_uri);
	if (f == nullptr)
		return;
	f->parse(*this);
}

void workspace::did_change_file(const std::string file_uri, const document_change * changes, size_t ch_size)
{
	if (file_uri == uri_ + "proc_grps.json" || file_uri == uri_ + "pgm_conf.json")
	{
		load_config();
		return;
	}

	auto f = file_manager_.find_processor_file(file_uri);
	if (f == nullptr)
		return;
	f->parse(*this);
}

void hlasm_plugin::parser_library::workspace::open()
{
	if (opened_)
		return;
	opened_ = load_config();
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
bool hlasm_plugin::parser_library::workspace::load_config()
{
	std::filesystem::path ws_path(uri_);
	using json = nlohmann::json;

	//proc_grps.json parse
	file * proc_grps_file = file_manager_.add_file((ws_path / FILENAME_PROC_GRPS).string());

	json proc_grps_json;
	try
	{
		proc_grps_json = nlohmann::json::parse(proc_grps_file->get_text());
	}
	catch(nlohmann::json::exception e)
	{
		add_diagnostic(diagnostic_s(uri_, {}, diagnostic_severity::error, 
			"W0002", "HLASM plugin",
			"The configuration file proc_grps for workspace " + name_ + " is malformed.", {}));
		return false;
	}

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
					prc_grp.add_library(std::make_unique<library_local>(file_manager_, lib_path.lexically_normal().string()));
				else if (lib_path.is_relative())
					prc_grp.add_library(std::make_unique<library_local>(file_manager_, (ws_path / lib_path).lexically_normal().string()));
				//else ignore, publish warning
			}
		}

		add_proc_grp(std::move(prc_grp));
	}


	//pgm_conf.json parse
	file * pgm_conf_file = file_manager_.add_file((ws_path / FILENAME_PGM_CONF).string());
	//files_.emplace(pgm_conf_file->get_file_name(), &pgm_conf_file);

	json pgm_conf_json;

	try
	{
		pgm_conf_json = nlohmann::json::parse(pgm_conf_file->get_text());
	}
	catch (nlohmann::json::exception e)
	{
		add_diagnostic(diagnostic_s(pgm_conf_file->get_file_name(), {}, diagnostic_severity::error,
			"W0003", "HLASM plugin",
			"The configuration file pgmp_conf for workspace " + name_ + " is malformed.", {}));
		return false;
	}
	json pgms = pgm_conf_json["pgms"];

	for (auto & pgm : pgms)
	{
		const std::string & program = pgm["program"].get<std::string>();
		const std::string & pgroup = pgm["pgroup"].get<std::string>();

		if (proc_grps_.find(pgroup) != proc_grps_.end())
		{
			pgm_conf_.emplace_back(std::move(program), std::move(pgroup));
		}
		else
		{ 
			add_diagnostic(diagnostic_s(pgm_conf_file->get_file_name(), {}, diagnostic_severity::warning,
				"W0004", "HLASM plugin",
				"The configuration file pgm_conf for workspace " + name_ + " refers to a processor group, that is not defined in proc_grps", {}));
		}
	}
	return true;
}

program_context * workspace::parse_library(const std::string & caller, const std::string & library, std::shared_ptr<context::hlasm_context> ctx)
{
	auto & proc_grp = get_proc_grp_by_program(caller);
	for (auto && lib : proc_grp.libraries())
	{
		processor * found = lib->find_file(library);
		if (found)
			return found->parse(*this, ctx);
	}
	
	return nullptr;
}


}
}
