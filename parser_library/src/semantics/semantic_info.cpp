#include "semantic_info.h"

namespace hlasm_plugin {
	namespace parser_library {
		namespace semantics {

			void semantic_info::merge(semantic_info second)
			{
				seq_symbols.add(second.seq_symbols);
				var_symbols.add(second.var_symbols);
				ord_symbols.add(second.ord_symbols);
      			continuation_positions.insert(continuation_positions.end(), second.continuation_positions.begin(), second.continuation_positions.end());
				hl_info.lines.insert(hl_info.lines.end(), second.hl_info.lines.begin(), second.hl_info.lines.end());
			}

			bool semantic_info::find_in_category(const position& pos, const symbol_info & current_symbols, position_uri_s & output)
			{
				for (auto occ : current_symbols.occurences)
					if (occ.range.begin_ln == pos.line && occ.range.end_ln == pos.line && occ.range.begin_col <= pos.column && occ.range.end_col >= pos.column)
					{
						auto corr_definition = std::find_if(current_symbols.definitions.begin(), current_symbols.definitions.end(), [&occ](auto def) { return def.name == occ.name && def.scope == occ.scope; });
						if (corr_definition != current_symbols.definitions.end())
						{
							output = { corr_definition->uri, position(corr_definition->range.begin_ln, corr_definition->range.begin_col) };
							return true;
						}				
					}
				return false;
			}

			position_uri_s semantic_info::find(const position & pos)
			{
				position_uri_s to_ret("", pos);
				if (find_in_category(pos, seq_symbols, to_ret) || find_in_category(pos, var_symbols, to_ret) || find_in_category(pos, ord_symbols, to_ret))
					return to_ret;
				return to_ret;
			}

			bool semantic_info::find_all_in_category(const position& pos, const symbol_info & current_symbols, std::vector<position_uri_s> & output)
			{
				symbol_occurence symbol;

				for (auto occ : current_symbols.occurences)
				{
					if (occ.range.begin_ln == pos.line && occ.range.end_ln == pos.line && occ.range.begin_col <= pos.column && occ.range.end_col >= pos.column)
					{
						symbol = occ;
						break;
					}
				}

				if (symbol.name == "")
				{
					for (auto def : current_symbols.definitions)
						if (def.range.begin_ln == pos.line && def.range.end_ln == pos.line && def.range.begin_col <= pos.column && def.range.end_col >= pos.column)
						{
							symbol = def;
							break;
						}
				}

				if (symbol.name != "")
				{
					for (auto occ : current_symbols.occurences)
						if (occ.name == symbol.name)
						{
							output.push_back({ occ.uri, position(occ.range.begin_ln, occ.range.begin_col) });
						}
					for (auto def : current_symbols.definitions)
						if (def.name == symbol.name)
						{
							output.push_back({ def.uri, position(def.range.begin_ln, def.range.begin_col) });
						}
					return true;
				}
				return false;
			}

			std::vector<position_uri_s> semantic_info::find_all(const position & pos)
			{
				std::vector<position_uri_s> to_ret;

				if (find_all_in_category(pos, seq_symbols, to_ret) || find_all_in_category(pos, var_symbols, to_ret) || find_all_in_category(pos, ord_symbols, to_ret))
					return to_ret;
				return to_ret;
			}

			void semantic_info::clear()
			{
				seq_symbols.clear();
				var_symbols.clear();
				ord_symbols.clear();
				continuation_positions.clear();
				hl_info.lines.clear();
			}

			void symbol_info::push_back(const symbol_occurence & occurence, bool is_definition)
			{
				if (std::find(definitions.begin(), definitions.end(), occurence) == definitions.end() && std::find(occurences.begin(), occurences.end(), occurence) == occurences.end())
				{
					if (is_definition && std::all_of(definitions.begin(), definitions.end(), [&occurence](auto def) {return def.name != occurence.name || def.scope != occurence.scope;  }))
					{
						definitions.push_back(occurence);
					}
					else
					{
						occurences.push_back(occurence);
					}
				}
			};

			void symbol_info::add(symbol_info info)
			{
				auto occ_list = info.occurences;
				occurences.insert(occurences.end(), occ_list.begin(), occ_list.end());
				auto def_list = info.definitions;
				definitions.insert(definitions.end(), def_list.begin(), def_list.end());
			};

			void symbol_info::clear()
			{
				occurences.clear();
				definitions.clear();
			}
		}
	}
}