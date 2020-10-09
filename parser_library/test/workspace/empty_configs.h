#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_EMPTY_CONFIGS_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_EMPTY_CONFIGS_H

inline std::string pgm_conf_name = (std::filesystem::path(".hlasmplugin") / "pgm_conf.json").string();
inline std::string proc_grps_name = (std::filesystem::path(".hlasmplugin") / "proc_grps.json").string();
inline std::string empty_pgm_conf = R"({ "pgms": []})";
inline std::string empty_proc_grps = R"({ "pgroups": []})";

#endif // !HLASMPLUGIN_PARSERLIBRARY_TEST_EMPTY_CONFIGS_H


