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

#ifndef HLASMPLUGIN_BENCHMARK_TEST_H
#define HLASMPLUGIN_BENCHMARK_TEST_H

#include "../include/shared/workspace_manager.h"
#include "../src/parse_lib_provider.h"
#include "../src/analyzer.h"
#include <memory>
using namespace hlasm_plugin::parser_library;

class metrics_mock : public performance_metrics_consumer
{
public:
	virtual void consume_performance_metrics(const performance_metrics& metrics)
	{
		metrics_ = metrics;
	}

	performance_metrics metrics_;
};

class diagnostic_counter_mock : public hlasm_plugin::parser_library::diagnostics_consumer
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

constexpr const char* MACRO_FILE = "MAC";
constexpr const char* SOURCE_FILE = "OPEN";
constexpr const char* COPY_FILE = "COPYFILE";

class mock_parse_lib_provider : public parse_lib_provider
{
public:
	virtual parse_result parse_library(const std::string& library, context::hlasm_context& hlasm_ctx, const library_data data) override
	{
		if (data.proc_kind == processing::processing_kind::MACRO)
		{
			analyzer a(macro_contents, MACRO_FILE, hlasm_ctx, *this, data);
			a.analyze();
		}
		else
		{
			analyzer a(copy_contents, COPY_FILE, hlasm_ctx, *this, data);
			a.analyze();
		}
		return true;
	}
	virtual bool has_library(const std::string&, context::hlasm_context&) const override { return true; }
private:
	const std::string macro_contents =
		R"(   MACRO
       MAC   &VAR
       LR    &VAR,&VAR
       MEND
)";
	const std::string copy_contents =
		R"(R2 EQU 2
			LR R2,R2)";
};

class benchmark_test : public testing::Test
{
public:
	benchmark_test() {};
	virtual void SetUp() {}
	virtual void TearDown() {}
	void setUpAnalyzer(const std::string& content)
	{
		a = std::make_unique<analyzer>(content, SOURCE_FILE, lib_provider);
		a->analyze();
	}
protected:
	mock_parse_lib_provider lib_provider;
	std::unique_ptr<analyzer> a;
};

TEST_F(benchmark_test, lines)
{
	setUpAnalyzer("a\nb\nc\nd");
	EXPECT_EQ(a->get_metrics().lines, (size_t)4);

	setUpAnalyzer("\n");
	// also counts empty lines as lines
	EXPECT_EQ(a->get_metrics().lines, (size_t)2);

	setUpAnalyzer(" LR 1,1\n MAC 1\n COPY COPYFILE");
	// 3 open code + 2 copy + 5 macro
	EXPECT_EQ(a->get_metrics().lines, (size_t)10);
}

TEST_F(benchmark_test, macro_statements)
{
	setUpAnalyzer(" MAC 1");
	// executed macro statements do not include MACRO at the beginning
	EXPECT_EQ(a->get_metrics().macro_statements, (size_t)3);
	// macro def statement do
	EXPECT_EQ(a->get_metrics().macro_def_statements, (size_t)4);

	setUpAnalyzer(" MAC 1\n MAC 2\n");
	EXPECT_EQ(a->get_metrics().macro_statements, (size_t)6);
	EXPECT_EQ(a->get_metrics().macro_def_statements, (size_t)4);

	setUpAnalyzer(" LR 1,1");
	EXPECT_EQ(a->get_metrics().macro_statements, (size_t)0);
	EXPECT_EQ(a->get_metrics().macro_def_statements, (size_t)0);
}

TEST_F(benchmark_test, copy_statements)
{
	setUpAnalyzer(" COPY COPYFILE");
	// 2 actual statements and 1 last ending statement
	EXPECT_EQ(a->get_metrics().copy_statements, (size_t)3);
	EXPECT_EQ(a->get_metrics().copy_def_statements, (size_t)3);

	setUpAnalyzer(" COPY COPYFILE\n COPY COPYFILE");
	EXPECT_EQ(a->get_metrics().copy_statements, (size_t)6);
	EXPECT_EQ(a->get_metrics().copy_def_statements, (size_t)3);

	setUpAnalyzer(" LR 1,1");
	EXPECT_EQ(a->get_metrics().copy_statements, (size_t)0);
	EXPECT_EQ(a->get_metrics().copy_def_statements, (size_t)0);
}

TEST_F(benchmark_test, open_code_statements)
{
	setUpAnalyzer(" COPY COPYFILE\n LR 1,1\n");
	// 2 actual statements and 1 empty and 1 ending statements
	EXPECT_EQ(a->get_metrics().open_code_statements, (size_t)4);
}

TEST_F(benchmark_test, continued_statements)
{
	setUpAnalyzer(R"(LABEL  LR    2,10 REMARK                                               X
                        second remark                                  X
                        third)");
	// only one long continued statement
	EXPECT_EQ(a->get_metrics().continued_statements, (size_t)1);
	EXPECT_EQ(a->get_metrics().non_continued_statements, (size_t)0);
}

TEST_F(benchmark_test, files)
{
	setUpAnalyzer(" MAC\n COPY COPYFILE\n MAC");
	// only 3 files visited -> macro, copy and open code, each once
	EXPECT_EQ(a->get_metrics().files, (size_t)3);
}

TEST_F(benchmark_test, reparsed_statements)
{
	setUpAnalyzer(" MAC\n");
	// 2 statements in MAC are reparsed
	EXPECT_EQ(a->get_metrics().reparsed_statements, (size_t)2);

	setUpAnalyzer(" MAC\n COPY COPYFILE");
	// 2 statements in MAC + 2 statements in COPYFILE are reparsed 
	EXPECT_EQ(a->get_metrics().reparsed_statements, (size_t)4);
}

TEST_F(benchmark_test, lookahead_statements)
{
	setUpAnalyzer(" AGO .HERE\n something\n something\n.HERE ANOP");
	// 2 lines skipped by lookahead + 1 which finds the symbol
	EXPECT_EQ(a->get_metrics().lookahead_statements, (size_t)3);
}
#endif