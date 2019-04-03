#pragma once
#include "semantic_highlighting_info.h"
//#include "../context/variable.h"
//#include "semantic_objects.h"
#include <vector>

namespace hlasm_plugin {
	namespace parser_library {
		namespace semantics {

		struct symbol_occurence
		{
			symbol_occurence() {};
			symbol_occurence(const symbol_range & range, const std::string & name, const std::string & uri, const std::string & scope) : uri(uri), range(range), name(name),  scope(scope) {};
			symbol_occurence(const symbol_range & range, const std::string & name, const std::string & uri, const std::string & scope, bool candidate) : uri(uri), range(range), name(name), scope(scope), candidate(candidate) {};
			bool operator==(const symbol_occurence & s) const
			{
				return uri == s.uri && range == s.range && name == s.name && scope == s.scope;
			}
			std::string uri;
			symbol_range range;
			std::string name;
			std::string scope;
			bool candidate = false;
		};

		class symbol_info
		{
		public:
			symbol_info() {};
			void push_back(const symbol_occurence & occurence, bool is_definition);
			void add(symbol_info info);
			void clear();
			std::vector<symbol_occurence> occurences;
			std::vector<symbol_occurence> definitions;
		};


		using positions = std::vector<hlasm_plugin::parser_library::position>;

		class semantic_info
		{
		public:
			semantic_info(std::string document_uri) : hl_info(document_uri)
			{
				//hl_info = semantic_highlighting_info();
			}
			semantic_info() {};

			hlasm_plugin::parser_library::semantics::semantic_highlighting_info hl_info;
			symbol_info seq_symbols;
			symbol_info var_symbols;
			symbol_info ord_symbols;
			positions continuation_positions;
			size_t continue_column;
			size_t continuation_column;

			position_uri_s find(const position & pos);
			std::vector<position_uri_s> find_all(const position & pos);
			void merge(semantic_info second);
			void clear();
		private:
			bool find_in_category(const position& pos, const symbol_info & current_symbols, position_uri_s & output);
			bool find_all_in_category(const position& pos, const symbol_info & current_symbols, std::vector<position_uri_s> & output);
		};

		class position_uri_s {
		public:
			position_uri_s() {};
			position_uri_s(std::string uri, position position) : uri(uri), pos(position) {};
			std::string uri;
			position pos;
		};
		}
	}
}