#include "gtest/gtest.h"

#include "analyzer_fixture.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;

struct lsp_context_document_symbol_ord_sect_1 : analyzer_fixture
{
    const static inline std::string input =
        R"(SEC0 CSECT
AUX  LR    1,1
E    EQU   1
SEC1 DSECT
)";
    lsp_context_document_symbol_ord_sect_1()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_document_symbol_ord_sect_1, sect_1)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string SEC0 = "SEC0", SEC1 = "SEC1", AUX = "AUX", E = "E";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s { E, document_symbol_kind::EQU, range { { 2, 0 }, { 2, 0 } } },
        document_symbol_item_s { SEC1, document_symbol_kind::DUMMY, range { { 3, 0 }, { 3, 0 } } },
        document_symbol_item_s { SEC0,
            document_symbol_kind::EXECUTABLE,
            range { { 0, 0 }, { 0, 0 } },
            document_symbol_list_s { document_symbol_item_s {
                AUX, document_symbol_kind::MACH, range { { 1, 5 }, { 1, 5 } } } } }
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

struct lsp_context_document_symbol_ord_sect_2 : analyzer_fixture
{
    const static inline std::string input =
        R"(SEC0 CSECT
AUX0 LR    1,1
E    EQU   1
SEC1 DSECT
AUX1 LR    1,1
SEC0 CSECT
AUX2 LR    1,1
SEC1 DSECT
AUX3 LR    1,1
)";
    lsp_context_document_symbol_ord_sect_2()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_document_symbol_ord_sect_2, sect_2)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string SEC0 = "SEC0", SEC1 = "SEC1", AUX0 = "AUX0", AUX1 = "AUX1", AUX2 = "AUX2", AUX3 = "AUX3", E = "E";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s { E, document_symbol_kind::EQU, range { { 2, 0 }, { 2, 0 } } },
        document_symbol_item_s { SEC1,
            document_symbol_kind::DUMMY,
            range { { 3, 0 }, { 3, 0 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    AUX3, document_symbol_kind::MACH, range { { 8, 5 }, { 8, 5 } } },
                document_symbol_item_s {
                    AUX1, document_symbol_kind::MACH, range { { 4, 5 }, { 4, 5 } } } } },
        document_symbol_item_s { SEC0,
            document_symbol_kind::EXECUTABLE,
            range { { 0, 0 }, { 0, 0 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    AUX2, document_symbol_kind::MACH, range { { 6, 5 }, { 6, 5 } } },
                document_symbol_item_s {
                    AUX0, document_symbol_kind::MACH, range { { 1, 5 }, { 1, 5 } } } } }
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

struct lsp_context_document_symbol_ord_macro_1 : analyzer_fixture
{
    const static inline std::string input =
        R"(    MACRO
    MAC
E   EQU 1
    MEND
)";
    lsp_context_document_symbol_ord_macro_1()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_document_symbol_ord_macro_1, macro_1)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    document_symbol_list_s expected = {};
    EXPECT_EQ(outline, expected);
}

struct lsp_context_document_symbol_ord_macro_2 : analyzer_fixture
{
    const static inline std::string input =
        R"(    MACRO
    MAC
AUX LR  1,1
E   EQU 1
    MEND
    MAC
)";
    lsp_context_document_symbol_ord_macro_2()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_document_symbol_ord_macro_2, macro_2)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string MAC = "MAC", E = "E", AUX = "AUX";
    document_symbol_list_s expected = document_symbol_list_s { document_symbol_item_s { MAC,
        document_symbol_kind::MACRO,
        range { { 5, 4 }, { 5, 4 } },
        document_symbol_list_s {
            document_symbol_item_s { E, document_symbol_kind::EQU, range { { 5, 4 }, { 5, 4 } } },
            document_symbol_item_s {
                AUX, document_symbol_kind::MACH, range { { 5, 4 }, { 5, 4 } } } } } };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

struct lsp_context_document_symbol_ord_macro_3 : analyzer_fixture
{
    const static inline std::string input =
        R"(     MACRO
     MAC1
E1   EQU 1
AUX1 LR  1,1
     MEND
    
     MACRO
     MAC2
E2   EQU 1
AUX2 LR  1,1
     MAC1
     MEND

     MAC2
)";
    lsp_context_document_symbol_ord_macro_3()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_document_symbol_ord_macro_3, macro_3)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string MAC1 = "MAC1", MAC2 = "MAC2", E1 = "E1", E2 = "E2", AUX1 = "AUX1", AUX2 = "AUX2";
    document_symbol_list_s expected = document_symbol_list_s { document_symbol_item_s { MAC2,
        document_symbol_kind::MACRO,
        range { { 13, 5 }, { 13, 5 } },
        document_symbol_list_s {
            document_symbol_item_s { MAC1,
                document_symbol_kind::MACRO,
                range { { 13, 5 }, { 13, 5 } },
                document_symbol_list_s {
                    document_symbol_item_s {
                        E1, document_symbol_kind::EQU, range { { 13, 5 }, { 13, 5 } } },
                    document_symbol_item_s {
                        AUX1, document_symbol_kind::MACH, range { { 13, 5 }, { 13, 5 } } } } },
            document_symbol_item_s { AUX2, document_symbol_kind::MACH, range { { 13, 5 }, { 13, 5 } } },
            document_symbol_item_s {
                E2, document_symbol_kind::EQU, range { { 13, 5 }, { 13, 5 } } } } } };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

struct lsp_context_document_symbol_ord_macro_4 : analyzer_fixture
{
    const static inline std::string input =
        R"(     MACRO
     MAC1
E1   EQU 1  
AUX1 LR  1,1
     MEND

     MACRO
     MAC2
     MAC1
E2   EQU 1  
AUX2 LR  1,1
     MEND

SEC  CSECT
AUX3 LR 1,1
     MAC2
)";
    lsp_context_document_symbol_ord_macro_4()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_document_symbol_ord_macro_4, macro_4)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string MAC1 = "MAC1", E1 = "E1", MAC2 = "MAC2", E2 = "E2", AUX1 = "AUX1", AUX2 = "AUX2", AUX3 = "AUX3",
                SEC = "SEC";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s { MAC2,
            document_symbol_kind::MACRO,
            range { { 15, 5 }, { 15, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    E2, document_symbol_kind::EQU, range { { 15, 5 }, { 15, 5 } } },
                document_symbol_item_s { MAC1,
                    document_symbol_kind::MACRO,
                    range { { 15, 5 }, { 15, 5 } },
                    document_symbol_list_s { document_symbol_item_s {
                        E1, document_symbol_kind::EQU, range { { 15, 5 }, { 15, 5 } } } } } } },
        document_symbol_item_s { SEC,
            document_symbol_kind::EXECUTABLE,
            range { { 13, 0 }, { 13, 0 } },
            document_symbol_list_s { document_symbol_item_s { MAC2,
                                         document_symbol_kind::MACRO,
                                         range { { 15, 5 }, { 15, 5 } },
                                         document_symbol_list_s { document_symbol_item_s { AUX2,
                                                                      document_symbol_kind::MACH,
                                                                      range { { 15, 5 }, { 15, 5 } } },
                                             document_symbol_item_s { MAC1,
                                                 document_symbol_kind::MACRO,
                                                 range { { 15, 5 }, { 15, 5 } },
                                                 document_symbol_list_s { document_symbol_item_s { AUX1,
                                                     document_symbol_kind::MACH,
                                                     range { { 15, 5 }, { 15, 5 } } } } } } },
                document_symbol_item_s {
                    AUX3, document_symbol_kind::MACH, range { { 14, 5 }, { 14, 5 } } } } }
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

struct lsp_context_document_symbol_ord_macro_5 : analyzer_fixture
{
    const static inline std::string input =
        R"(     MACRO
     MAC1
E1   EQU 1
AUX1 LR  1,1
     MEND

     MACRO
     MAC2
E2   EQU 1
AUX2 LR  1,1
SEC1 CSECT
     MAC1
AUX3 LR  1,1
     MEND

SEC2 CSECT
AUX4 LR 1,1
     MAC2
E3   EQU 1
)";
    lsp_context_document_symbol_ord_macro_5()
        : analyzer_fixture(input)
    {}
};

TEST_F(lsp_context_document_symbol_ord_macro_5, macro_5)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string MAC1 = "MAC1", MAC2 = "MAC2", E1 = "E1", E2 = "E2", E3 = "E3", AUX1 = "AUX1", AUX2 = "AUX2",
                AUX3 = "AUX3", AUX4 = "AUX4", SEC1 = "SEC1", SEC2 = "SEC2";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s { E3, document_symbol_kind::EQU, range { { 18, 0 }, { 18, 0 } } },
        document_symbol_item_s { MAC2,
            document_symbol_kind::MACRO,
            range { { 17, 5 }, { 17, 5 } },
            document_symbol_list_s {
                document_symbol_item_s { MAC1,
                    document_symbol_kind::MACRO,
                    range { { 17, 5 }, { 17, 5 } },
                    document_symbol_list_s { document_symbol_item_s {
                        E1, document_symbol_kind::EQU, range { { 17, 5 }, { 17, 5 } } } } },
                document_symbol_item_s {
                    E2, document_symbol_kind::EQU, range { { 17, 5 }, { 17, 5 } } },
                document_symbol_item_s { SEC1,
                    document_symbol_kind::EXECUTABLE,
                    range { { 17, 5 }, { 17, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            AUX3, document_symbol_kind::MACH, range { { 17, 5 }, { 17, 5 } } },
                        document_symbol_item_s { MAC1,
                            document_symbol_kind::MACRO,
                            range { { 17, 5 }, { 17, 5 } },
                            document_symbol_list_s { document_symbol_item_s { AUX1,
                                document_symbol_kind::MACH,
                                range { { 17, 5 }, { 17, 5 } } } } } } } } },
        document_symbol_item_s { SEC2,
            document_symbol_kind::EXECUTABLE,
            range { { 15, 0 }, { 15, 0 } },
            document_symbol_list_s {
                document_symbol_item_s { MAC2,
                    document_symbol_kind::MACRO,
                    range { { 17, 5 }, { 17, 5 } },
                    document_symbol_list_s { document_symbol_item_s {
                        AUX2, document_symbol_kind::MACH, range { { 17, 5 }, { 17, 5 } } } } },
                document_symbol_item_s {
                    AUX4, document_symbol_kind::MACH, range { { 16, 5 }, { 16, 5 } } } } }
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

struct lsp_context_document_symbol_ord_macro_6 : public analyzer_fixture
{
    const static inline std::string opencode =
        R"(    MAC1
)";
    const static inline std::string macro_file_name = "MAC1";
    const static inline std::string macro =
        R"(    MACRO
    MAC1
E   EQU 1
AUX LR  1,1
    MEND
)";
    class lib_provider_mock : public workspaces::parse_lib_provider
    {
        asm_option empty_options;
        workspaces::parse_result parse_library(
            const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
        {
            const std::string* text;
            if (library == macro_file_name)
                text = &macro;
            else
                return false;

            analyzer lib_analyzer(*text, library, ctx, *this, data);
            lib_analyzer.analyze();
            return true;
        };

        bool has_library(const std::string& library, const std::string&) const override
        {
            return library == macro_file_name;
        };

        const asm_option& get_asm_options(const std::string&) override { return empty_options; };
    };
    static inline lib_provider_mock lib_prov_instance;
    lsp_context_document_symbol_ord_macro_6()
        : analyzer_fixture(opencode, lib_prov_instance)
    {}
};

TEST_F(lsp_context_document_symbol_ord_macro_6, macro_6)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string MAC1 = "MAC1", AUX = "AUX", E = "E";
    document_symbol_list_s expected = document_symbol_list_s { document_symbol_item_s { MAC1,
        document_symbol_kind::MACRO,
        range { { 0, 4 }, { 0, 4 } },
        document_symbol_list_s {
            document_symbol_item_s { AUX, document_symbol_kind::MACH, range { { 0, 4 }, { 0, 4 } } },
            document_symbol_item_s { E, document_symbol_kind::EQU, range { { 0, 4 }, { 0, 4 } } } } } };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

struct lsp_context_document_symbol_ord_macro_7 : public analyzer_fixture
{
    const static inline std::string opencode =
        R"(     MACRO
     MAC0
AUX0 LR   1,1
E0   EQU  1
     MAC1
     MEND

SEC0 CSECT
     MAC0
)";
    const static inline std::string macro_file_name1 = "MAC1", macro_file_name2 = "MAC2";
    const static inline std::string macro1 =
                                        R"(     MACRO
     MAC1
AUX1 LR   1,1
SEC1 DSECT
E1   EQU  1 
     MAC2
     MEND
)",
                                    macro2 =
                                        R"(     MACRO
     MAC2
AUX2 LR   1,1
E2   EQU  1
     MEND
)";
    class lib_provider_mock : public workspaces::parse_lib_provider
    {
        asm_option empty_options;
        workspaces::parse_result parse_library(
            const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
        {
            const std::string* text;
            if (library == macro_file_name1)
                text = &macro1;
            else if (library == macro_file_name2)
                text = &macro2;
            else
                return false;

            analyzer lib_analyzer(*text, library, ctx, *this, data);
            lib_analyzer.analyze();
            return true;
        };

        bool has_library(const std::string& library, const std::string&) const override
        {
            return library == macro_file_name1 || library == macro_file_name2;
        };

        const asm_option& get_asm_options(const std::string&) override { return empty_options; };
    };
    static inline lib_provider_mock lib_prov_instance;
    lsp_context_document_symbol_ord_macro_7()
        : analyzer_fixture(opencode, lib_prov_instance)
    {}
};

TEST_F(lsp_context_document_symbol_ord_macro_7, macro_7)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string MAC0 = "MAC0", AUX0 = "AUX0", MAC1 = "MAC1", AUX1 = "AUX1", MAC2 = "MAC2", AUX2 = "AUX2", SEC0 = "SEC0",
                SEC1 = "SEC1", E0 = "E0", E1 = "E1", E2 = "E2";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s { MAC0,
            document_symbol_kind::MACRO,
            range { { 8, 5 }, { 8, 5 } },
            document_symbol_list_s {
                document_symbol_item_s { MAC1,
                    document_symbol_kind::MACRO,
                    range { { 8, 5 }, { 8, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s { MAC2,
                            document_symbol_kind::MACRO,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s { document_symbol_item_s {
                                E2, document_symbol_kind::EQU, range { { 8, 5 }, { 8, 5 } } } } },
                        document_symbol_item_s {
                            E1, document_symbol_kind::EQU, range { { 8, 5 }, { 8, 5 } } },
                        document_symbol_item_s { SEC1,
                            document_symbol_kind::DUMMY,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s { document_symbol_item_s { MAC2,
                                document_symbol_kind::MACRO,
                                range { { 8, 5 }, { 8, 5 } },
                                document_symbol_list_s { document_symbol_item_s { AUX2,
                                    document_symbol_kind::MACH,
                                    range { { 8, 5 }, { 8, 5 } } } } } } } } },
                document_symbol_item_s {
                    E0, document_symbol_kind::EQU, range { { 8, 5 }, { 8, 5 } } } } },
        document_symbol_item_s { SEC0,
            document_symbol_kind::EXECUTABLE,
            range { { 7, 0 }, { 7, 0 } },
            document_symbol_list_s { document_symbol_item_s { MAC0,
                document_symbol_kind::MACRO,
                range { { 8, 5 }, { 8, 5 } },
                document_symbol_list_s {
                    document_symbol_item_s { MAC1,
                        document_symbol_kind::MACRO,
                        range { { 8, 5 }, { 8, 5 } },
                        document_symbol_list_s { document_symbol_item_s {
                            AUX1, document_symbol_kind::MACH, range { { 8, 5 }, { 8, 5 } } } } },
                    document_symbol_item_s {
                        AUX0, document_symbol_kind::MACH, range { { 8, 5 }, { 8, 5 } } } } } } }
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}


struct lsp_context_document_symbol_ord_macro_8 : public analyzer_fixture
{
    const static inline std::string opencode =
        R"(     MACRO
     MAC0
AUX0 LR   1,1
E0   EQU  1
     MAC1
     MEND

SEC0 CSECT
     MAC0
)";
    const static inline std::string macro_file_name1 = "MAC1", macro_file_name2 = "MAC2";
    const static inline std::string macro1 =
                                        R"(     MACRO
     MAC1
SEC1 DSECT
AUX1 LR   1,1
E1   EQU  1 
     MAC2
     MEND
)",
                                    macro2 =
                                        R"(     MACRO
     MAC2
SEC0 CSECT
AUX2 LR   1,1
E2   EQU  1
     MEND
)";
    class lib_provider_mock : public workspaces::parse_lib_provider
    {
        asm_option empty_options;
        workspaces::parse_result parse_library(
            const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
        {
            const std::string* text;
            if (library == macro_file_name1)
                text = &macro1;
            else if (library == macro_file_name2)
                text = &macro2;
            else
                return false;

            analyzer lib_analyzer(*text, library, ctx, *this, data);
            lib_analyzer.analyze();
            return true;
        };

        bool has_library(const std::string& library, const std::string&) const override
        {
            return library == macro_file_name1 || library == macro_file_name2;
        };

        const asm_option& get_asm_options(const std::string&) override { return empty_options; };
    };
    static inline lib_provider_mock lib_prov_instance;
    lsp_context_document_symbol_ord_macro_8()
        : analyzer_fixture(opencode, lib_prov_instance)
    {}
};

TEST_F(lsp_context_document_symbol_ord_macro_8, macro_8)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string MAC0 = "MAC0", AUX0 = "AUX0", MAC1 = "MAC1", AUX1 = "AUX1", MAC2 = "MAC2", AUX2 = "AUX2", SEC0 = "SEC0",
                SEC1 = "SEC1", E0 = "E0", E1 = "E1", E2 = "E2";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s { MAC0,
            document_symbol_kind::MACRO,
            range { { 8, 5 }, { 8, 5 } },
            document_symbol_list_s {
                document_symbol_item_s { MAC1,
                    document_symbol_kind::MACRO,
                    range { { 8, 5 }, { 8, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s { MAC2,
                            document_symbol_kind::MACRO,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s { document_symbol_item_s {
                                E2, document_symbol_kind::EQU, range { { 8, 5 }, { 8, 5 } } } } },
                        document_symbol_item_s {
                            E1, document_symbol_kind::EQU, range { { 8, 5 }, { 8, 5 } } },
                        document_symbol_item_s { SEC1,
                            document_symbol_kind::DUMMY,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s { document_symbol_item_s { AUX1,
                                document_symbol_kind::MACH,
                                range { { 8, 5 }, { 8, 5 } } } } } } },
                document_symbol_item_s {
                    E0, document_symbol_kind::EQU, range { { 8, 5 }, { 8, 5 } } } } },
        document_symbol_item_s { SEC0,
            document_symbol_kind::EXECUTABLE,
            range { { 7, 0 }, { 7, 0 } },
            document_symbol_list_s { document_symbol_item_s { MAC0,
                document_symbol_kind::MACRO,
                range { { 8, 5 }, { 8, 5 } },
                document_symbol_list_s { document_symbol_item_s { MAC1,
                                             document_symbol_kind::MACRO,
                                             range { { 8, 5 }, { 8, 5 } },
                                             document_symbol_list_s { document_symbol_item_s { MAC2,
                                                 document_symbol_kind::MACRO,
                                                 range { { 8, 5 }, { 8, 5 } },
                                                 document_symbol_list_s { document_symbol_item_s { AUX2,
                                                     document_symbol_kind::MACH,
                                                     range { { 8, 5 }, { 8, 5 } } } } } } },
                    document_symbol_item_s {
                        AUX0, document_symbol_kind::MACH, range { { 8, 5 }, { 8, 5 } } } } } } }
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

struct lsp_context_document_symbol_ord_macro_9 : public analyzer_fixture
{
    const static inline std::string opencode =
        R"(     MACRO
     MAC0
AUX0 LR   1,1
E0   EQU  1
     MAC1
     MEND

     MAC0
SEC0 DSECT   
     MAC3
)";
    const static inline std::string macro_file_name1 = "MAC1", macro_file_name2 = "MAC2", macro_file_name3 = "MAC3",
                                    macro_file_name4 = "MAC4";
    const static inline std::string macro1 =
                                        R"(     MACRO
     MAC1
SEC1 CSECT
AUX1 LR   1,1
E1   EQU  1 
     MEND
)",
                                    macro2 =
                                        R"(     MACRO
     MAC2
AUX2 LR   1,1
E2   EQU  1
     MAC2
     MEND
)",
                                    macro3 =
                                        R"(     MACRO
     MAC3
AUX3 LR   1,1
E3   EQU  1 
     MAC4
     MEND
)",
                                    macro4 =
                                        R"(     MACRO
     MAC4
SEC1 CSECT
AUX4 LR   1,1
E4   EQU  1 
     MEND
)";
    ;
    class lib_provider_mock : public workspaces::parse_lib_provider
    {
        asm_option empty_options;
        workspaces::parse_result parse_library(
            const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
        {
            const std::string* text;
            if (library == macro_file_name1)
                text = &macro1;
            else if (library == macro_file_name2)
                text = &macro2;
            else if (library == macro_file_name3)
                text = &macro3;
            else if (library == macro_file_name4)
                text = &macro4;
            else
                return false;

            analyzer lib_analyzer(*text, library, ctx, *this, data);
            lib_analyzer.analyze();
            return true;
        };

        bool has_library(const std::string& library, const std::string&) const override
        {
            return library == macro_file_name1 || library == macro_file_name2 || library == macro_file_name3
                || library == macro_file_name4;
        };

        const asm_option& get_asm_options(const std::string&) override { return empty_options; };
    };
    static inline lib_provider_mock lib_prov_instance;
    lsp_context_document_symbol_ord_macro_9()
        : analyzer_fixture(opencode, lib_prov_instance)
    {}
};

TEST_F(lsp_context_document_symbol_ord_macro_9, macro_9)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string MAC0 = "MAC0", AUX0 = "AUX0", MAC1 = "MAC1", AUX1 = "AUX1", MAC2 = "MAC2", AUX2 = "AUX2", SEC0 = "SEC0",
                SEC1 = "SEC1", E0 = "E0", E1 = "E1", E2 = "E2", MAC3 = "MAC3", AUX3 = "AUX3", E3 = "E3", MAC4 = "MAC4",
                AUX4 = "AUX4", E4 = "E4";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s { MAC3,
            document_symbol_kind::MACRO,
            range { { 9, 5 }, { 9, 5 } },
            document_symbol_list_s {
                document_symbol_item_s { MAC4,
                    document_symbol_kind::MACRO,
                    range { { 9, 5 }, { 9, 5 } },
                    document_symbol_list_s { document_symbol_item_s {
                        E4, document_symbol_kind::EQU, range { { 9, 5 }, { 9, 5 } } } } },
                document_symbol_item_s {
                    E3, document_symbol_kind::EQU, range { { 9, 5 }, { 9, 5 } } } } },
        document_symbol_item_s { MAC0,
            document_symbol_kind::MACRO,
            range { { 7, 5 }, { 7, 5 } },
            document_symbol_list_s {
                document_symbol_item_s { MAC1,
                    document_symbol_kind::MACRO,
                    range { { 7, 5 }, { 7, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            E1, document_symbol_kind::EQU, range { { 7, 5 }, { 7, 5 } } },
                        document_symbol_item_s { SEC1,
                            document_symbol_kind::EXECUTABLE,
                            range { { 7, 5 }, { 7, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s { MAC4,
                                    document_symbol_kind::MACRO,
                                    range { { 9, 5 }, { 9, 5 } },
                                    document_symbol_list_s { document_symbol_item_s { AUX4,
                                        document_symbol_kind::MACH,
                                        range { { 9, 5 }, { 9, 5 } } } } },
                                document_symbol_item_s { AUX1,
                                    document_symbol_kind::MACH,
                                    range { { 7, 5 }, { 7, 5 } } } } } } },
                document_symbol_item_s { E0, document_symbol_kind::EQU, range { { 7, 5 }, { 7, 5 } } },
                document_symbol_item_s {
                    AUX0, document_symbol_kind::MACH, range { { 7, 5 }, { 7, 5 } } } } },
        document_symbol_item_s { SEC0,
            document_symbol_kind::DUMMY,
            range { { 8, 0 }, { 8, 0 } },
            document_symbol_list_s { document_symbol_item_s { MAC3,
                document_symbol_kind::MACRO,
                range { { 9, 5 }, { 9, 5 } },
                document_symbol_list_s { document_symbol_item_s {
                    AUX3, document_symbol_kind::MACH, range { { 9, 5 }, { 9, 5 } } } } } } }
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

struct lsp_context_document_symbol_ord_copy : public analyzer_fixture
{
    const static inline std::string opencode =
        R"(
    COPY COPYFILE1
)";
    const static inline std::string copyfile_file_name1 = "COPYFILE1";
    const static inline std::string copyfile1 = R"()";
    class lib_provider_mock : public workspaces::parse_lib_provider
    {
        asm_option empty_options;
        workspaces::parse_result parse_library(
            const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
        {
            const std::string* text;
            if (library == copyfile_file_name1)
                text = &copyfile1;
            else
                return false;

            analyzer lib_analyzer(*text, library, ctx, *this, data);
            lib_analyzer.analyze();
            return true;
        };

        bool has_library(const std::string& library, const std::string&) const override
        {
            return library == copyfile_file_name1;
        };

        const asm_option& get_asm_options(const std::string&) override { return empty_options; };
    };
    static inline lib_provider_mock lib_prov_instance;
    lsp_context_document_symbol_ord_copy()
        : analyzer_fixture(opencode, lib_prov_instance)
    {}
};

TEST_F(lsp_context_document_symbol_ord_copy, copy)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    document_symbol_list_s expected = document_symbol_list_s {};
    EXPECT_EQ(outline, expected);
}

struct lsp_context_document_symbol_ord_copy_2 : public analyzer_fixture
{
    const static inline std::string opencode =
        R"(E0   EQU  1
AUX0 LR   1,1
     COPY COPYFILE1
)";
    const static inline std::string copyfile_file_name1 = "COPYFILE1";
    const static inline std::string copyfile1 =
        R"(E1   EQU  1
AUX1 LR   1,1
)";
    class lib_provider_mock : public workspaces::parse_lib_provider
    {
        asm_option empty_options;
        workspaces::parse_result parse_library(
            const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
        {
            const std::string* text;
            if (library == copyfile_file_name1)
                text = &copyfile1;
            else
                return false;

            analyzer lib_analyzer(*text, library, ctx, *this, data);
            lib_analyzer.analyze();
            return true;
        };

        bool has_library(const std::string& library, const std::string&) const override
        {
            return library == copyfile_file_name1;
        };

        const asm_option& get_asm_options(const std::string&) override { return empty_options; };
    };
    static inline lib_provider_mock lib_prov_instance;
    lsp_context_document_symbol_ord_copy_2()
        : analyzer_fixture(opencode, lib_prov_instance)
    {}
};

TEST_F(lsp_context_document_symbol_ord_copy_2, copy_2)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string E0 = "E0", AUX0 = "AUX0", E1 = "E1", AUX1 = "AUX1", COPYFILE1 = "COPYFILE1";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s { COPYFILE1,
            document_symbol_kind::MACRO,
            range { { 2, 5 }, { 2, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    AUX1, document_symbol_kind::MACH, range { { 2, 5 }, { 2, 5 } } },
                document_symbol_item_s {
                    E1, document_symbol_kind::EQU, range { { 2, 5 }, { 2, 5 } } } } },
        document_symbol_item_s { AUX0, document_symbol_kind::MACH, range { { 1, 0 }, { 1, 0 } } },
        document_symbol_item_s { E0, document_symbol_kind::EQU, range { { 0, 0 }, { 0, 0 } } }
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

struct lsp_context_document_symbol_ord_copy_3 : public analyzer_fixture
{
    const static inline std::string opencode =
        R"(     MACRO
     MAC0
EM0  EQU   1
AM0  LR    1,1
     COPY  COPYFILE1
     MEND

     MAC0
     MAC1
)";
    const static inline std::string copyfile_file_name1 = "COPYFILE1", copyfile_file_name2 = "COPYFILE2",
                                    copyfile_file_name3 = "COPYFILE3";
    const static inline std::string copyfile1 =
                                        R"(EC1   EQU  1
SEC1  CSECT
AC1   LR   1,1
)",
                                    copyfile2 =
                                        R"(EC2  EQU  1
SEC2 DSECT
AC2  LR   1,1
     COPY COPYFILE3)",
                                    copyfile3 =
                                        R"(EC3   EQU  1
AC3   LR   1,1
)";
    const static inline std::string macro_file_name1 = "MAC1";
    const static inline std::string macro1 =
        R"(     MACRO
     MAC1
AM1  LR   1,1
EM1  EQU  1 
     COPY COPYFILE2
     MEND
)";
    class lib_provider_mock : public workspaces::parse_lib_provider
    {
        asm_option empty_options;
        workspaces::parse_result parse_library(
            const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
        {
            const std::string* text;
            if (library == copyfile_file_name1)
                text = &copyfile1;
            else if (library == copyfile_file_name2)
                text = &copyfile2;
            else if (library == copyfile_file_name3)
                text = &copyfile3;
            else if (library == macro_file_name1)
                text = &macro1;
            else
                return false;

            analyzer lib_analyzer(*text, library, ctx, *this, data);
            lib_analyzer.analyze();
            return true;
        };

        bool has_library(const std::string& library, const std::string&) const override
        {
            return library == copyfile_file_name1 || library == copyfile_file_name2 || library == copyfile_file_name3
                || library == macro_file_name1;
        };

        const asm_option& get_asm_options(const std::string&) override { return empty_options; };
    };
    static inline lib_provider_mock lib_prov_instance;
    lsp_context_document_symbol_ord_copy_3()
        : analyzer_fixture(opencode, lib_prov_instance)
    {}
};

TEST_F(lsp_context_document_symbol_ord_copy_3, copy_3)
{
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol(opencode_file_name);
    std::string EM0 = "EM0", AM0 = "AM0", EM1 = "EM1", AM1 = "AM1", EC0 = "EC0", AC0 = "AC0", EC1 = "EC1", AC1 = "AC1",
                EC2 = "EC2", AC2 = "AC2", EC3 = "EC3", AC3 = "AC3", COPYFILE1 = "COPYFILE1", COPYFILE2 = "COPYFILE2",
                COPYFILE3 = "COPYFILE3", MAC0 = "MAC0", MAC1 = "MAC1", SEC1 = "SEC1", SEC2 = "SEC2";
    document_symbol_list_s expected =
        document_symbol_list_s {
            document_symbol_item_s { MAC1,
                document_symbol_kind::MACRO,
                range { { 8, 5 }, { 8, 5 } },
                document_symbol_list_s {
                    document_symbol_item_s { COPYFILE2,
                        document_symbol_kind::MACRO,
                        range { { 8, 5 }, { 8, 5 } },
                        document_symbol_list_s {
                            document_symbol_item_s {
                                EC2, document_symbol_kind::EQU, range { { 8, 5 }, { 8, 5 } } },
                            document_symbol_item_s { COPYFILE3,
                                document_symbol_kind::MACRO,
                                range { { 8, 5 }, { 8, 5 } },
                                document_symbol_list_s { document_symbol_item_s {
                                    EC3, document_symbol_kind::EQU, range { { 8, 5 }, { 8, 5 } } } } },
                            document_symbol_item_s { SEC2,
                                document_symbol_kind::DUMMY,
                                range { { 8, 5 }, { 8, 5 } },
                                document_symbol_list_s {
                                    document_symbol_item_s { COPYFILE3,
                                        document_symbol_kind::MACRO,
                                        range { { 8, 5 }, { 8, 5 } },
                                        document_symbol_list_s { document_symbol_item_s { AC3,
                                            document_symbol_kind::MACH,
                                            range { { 8, 5 }, { 8, 5 } } } } },
                                    document_symbol_item_s { AC2,
                                        document_symbol_kind::MACH,
                                        range { { 8, 5 }, { 8, 5 } } } } } } },
                    document_symbol_item_s {
                        EM1, document_symbol_kind::EQU, range { { 8, 5 }, { 8, 5 } } } } },
            document_symbol_item_s { MAC0,
                document_symbol_kind::MACRO,
                range { { 7, 5 }, { 7, 5 } },
                document_symbol_list_s {
                    document_symbol_item_s { COPYFILE1,
                        document_symbol_kind::MACRO,
                        range { { 7, 5 }, { 7, 5 } },
                        document_symbol_list_s { document_symbol_item_s { EC1,
                                                     document_symbol_kind::EQU,
                                                     range { { 7, 5 }, { 7, 5 } } },
                            document_symbol_item_s { SEC1,
                                document_symbol_kind::EXECUTABLE,
                                range { { 7, 5 }, { 7, 5 } },
                                document_symbol_list_s { document_symbol_item_s { AM1,
                                                             document_symbol_kind::MACH,
                                                             range { { 8, 5 }, { 8, 5 } } },
                                    document_symbol_item_s { AC1,
                                        document_symbol_kind::MACH,
                                        range { { 7, 5 }, { 7, 5 } } } } } } },
                    document_symbol_item_s {
                        AM0, document_symbol_kind::MACH, range { { 7, 5 }, { 7, 5 } } },
                    document_symbol_item_s {
                        EM0, document_symbol_kind::EQU, range { { 7, 5 }, { 7, 5 } } } } }
        };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}
