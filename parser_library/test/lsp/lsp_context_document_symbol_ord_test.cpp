/*
 * Copyright (c) 2021 Broadcom.
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

#include "gtest/gtest.h"

#include "../mock_parse_lib_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;

TEST(lsp_context_document_symbol, ord_sect_1)
{
    std::string input =
        R"(SEC0 CSECT
AUX  LR    1,1
E    EQU   1
SEC1 DSECT
)";
    analyzer a(input);
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string SEC0 = "SEC0", SEC1 = "SEC1", AUX = "AUX", E = "E";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            E,
            document_symbol_kind::EQU,
            range { { 2, 0 }, { 2, 0 } },
        },
        document_symbol_item_s {
            SEC1,
            document_symbol_kind::DUMMY,
            range { { 3, 0 }, { 3, 0 } },
        },
        document_symbol_item_s {
            SEC0,
            document_symbol_kind::EXECUTABLE,
            range { { 0, 0 }, { 0, 0 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    AUX,
                    document_symbol_kind::MACH,
                    range { { 1, 5 }, { 1, 5 } },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_sect_2)
{
    std::string input =
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
    analyzer a(input);
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string SEC0 = "SEC0", SEC1 = "SEC1", AUX0 = "AUX0", AUX1 = "AUX1", AUX2 = "AUX2", AUX3 = "AUX3", E = "E";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            E,
            document_symbol_kind::EQU,
            range { { 2, 0 }, { 2, 0 } },
        },
        document_symbol_item_s {
            SEC1,
            document_symbol_kind::DUMMY,
            range { { 3, 0 }, { 3, 0 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    AUX3,
                    document_symbol_kind::MACH,
                    range { { 8, 5 }, { 8, 5 } },
                },
                document_symbol_item_s {
                    AUX1,
                    document_symbol_kind::MACH,
                    range { { 4, 5 }, { 4, 5 } },
                },
            },
        },
        document_symbol_item_s {
            SEC0,
            document_symbol_kind::EXECUTABLE,
            range { { 0, 0 }, { 0, 0 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    AUX2,
                    document_symbol_kind::MACH,
                    range { { 6, 5 }, { 6, 5 } },
                },
                document_symbol_item_s {
                    AUX0,
                    document_symbol_kind::MACH,
                    range { { 1, 5 }, { 1, 5 } },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_macro_1)
{
    std::string input =
        R"(    MACRO
    MAC
E   EQU 1
    MEND
)";
    analyzer a(input);
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    document_symbol_list_s expected = {};
    EXPECT_EQ(outline, expected);
}

TEST(lsp_context_document_symbol, ord_macro_2)
{
    std::string input =
        R"(    MACRO
    MAC
AUX LR  1,1
E   EQU 1
    MEND
    MAC
)";
    analyzer a(input);
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string MAC = "MAC", E = "E", AUX = "AUX";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            MAC,
            document_symbol_kind::MACRO,
            range { { 5, 4 }, { 5, 4 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    E,
                    document_symbol_kind::EQU,
                    range { { 5, 4 }, { 5, 4 } },
                },
                document_symbol_item_s {
                    AUX,
                    document_symbol_kind::MACH,
                    range { { 5, 4 }, { 5, 4 } },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_macro_3)
{
    std::string input =
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
    analyzer a(input);
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string MAC1 = "MAC1", MAC2 = "MAC2", E1 = "E1", E2 = "E2", AUX1 = "AUX1", AUX2 = "AUX2";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            MAC2,
            document_symbol_kind::MACRO,
            range { { 13, 5 }, { 13, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC1,
                    document_symbol_kind::MACRO,
                    range { { 13, 5 }, { 13, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            E1,
                            document_symbol_kind::EQU,
                            range { { 13, 5 }, { 13, 5 } },
                        },
                        document_symbol_item_s {
                            AUX1,
                            document_symbol_kind::MACH,
                            range { { 13, 5 }, { 13, 5 } },
                        },
                    },
                },
                document_symbol_item_s {
                    AUX2,
                    document_symbol_kind::MACH,
                    range { { 13, 5 }, { 13, 5 } },
                },
                document_symbol_item_s {
                    E2,
                    document_symbol_kind::EQU,
                    range { { 13, 5 }, { 13, 5 } },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_macro_4)
{
    std::string input =
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
    analyzer a(input);
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string MAC1 = "MAC1", E1 = "E1", MAC2 = "MAC2", E2 = "E2", AUX1 = "AUX1", AUX2 = "AUX2", AUX3 = "AUX3",
                SEC = "SEC";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            MAC2,
            document_symbol_kind::MACRO,
            range { { 15, 5 }, { 15, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    E2,
                    document_symbol_kind::EQU,
                    range { { 15, 5 }, { 15, 5 } },
                },
                document_symbol_item_s {
                    MAC1,
                    document_symbol_kind::MACRO,
                    range { { 15, 5 }, { 15, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            E1,
                            document_symbol_kind::EQU,
                            range { { 15, 5 }, { 15, 5 } },
                        },
                    },
                },
            },
        },
        document_symbol_item_s {
            SEC,
            document_symbol_kind::EXECUTABLE,
            range { { 13, 0 }, { 13, 0 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC2,
                    document_symbol_kind::MACRO,
                    range { { 15, 5 }, { 15, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            AUX2,
                            document_symbol_kind::MACH,
                            range { { 15, 5 }, { 15, 5 } },
                        },
                        document_symbol_item_s {
                            MAC1,
                            document_symbol_kind::MACRO,
                            range { { 15, 5 }, { 15, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    AUX1,
                                    document_symbol_kind::MACH,
                                    range { { 15, 5 }, { 15, 5 } },
                                },
                            },
                        },
                    },
                },
                document_symbol_item_s {
                    AUX3,
                    document_symbol_kind::MACH,
                    range { { 14, 5 }, { 14, 5 } },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_macro_5)
{
    std::string input =
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
    analyzer a(input);
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string MAC1 = "MAC1", MAC2 = "MAC2", E1 = "E1", E2 = "E2", E3 = "E3", AUX1 = "AUX1", AUX2 = "AUX2",
                AUX3 = "AUX3", AUX4 = "AUX4", SEC1 = "SEC1", SEC2 = "SEC2";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            E3,
            document_symbol_kind::EQU,
            range { { 18, 0 }, { 18, 0 } },
        },
        document_symbol_item_s {
            MAC2,
            document_symbol_kind::MACRO,
            range { { 17, 5 }, { 17, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC1,
                    document_symbol_kind::MACRO,
                    range { { 17, 5 }, { 17, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            E1,
                            document_symbol_kind::EQU,
                            range { { 17, 5 }, { 17, 5 } },
                        },
                    },
                },
                document_symbol_item_s {
                    E2,
                    document_symbol_kind::EQU,
                    range { { 17, 5 }, { 17, 5 } },
                },
                document_symbol_item_s {
                    SEC1,
                    document_symbol_kind::EXECUTABLE,
                    range { { 17, 5 }, { 17, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            AUX3,
                            document_symbol_kind::MACH,
                            range { { 17, 5 }, { 17, 5 } },
                        },
                        document_symbol_item_s {
                            MAC1,
                            document_symbol_kind::MACRO,
                            range { { 17, 5 }, { 17, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    AUX1,
                                    document_symbol_kind::MACH,
                                    range { { 17, 5 }, { 17, 5 } },
                                },
                            },
                        },
                    },
                },
            },
        },
        document_symbol_item_s {
            SEC2,
            document_symbol_kind::EXECUTABLE,
            range { { 15, 0 }, { 15, 0 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC2,
                    document_symbol_kind::MACRO,
                    range { { 17, 5 }, { 17, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            AUX2,
                            document_symbol_kind::MACH,
                            range { { 17, 5 }, { 17, 5 } },
                        },
                    },
                },
                document_symbol_item_s {
                    AUX4,
                    document_symbol_kind::MACH,
                    range { { 16, 5 }, { 16, 5 } },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_macro_6)
{
    std::string opencode =
        R"(    MAC1
)";
    mock_parse_lib_provider mock({
        { "MAC1", R"(    MACRO
    MAC1
E   EQU 1
AUX LR  1,1
    MEND
)" },
    });
    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string MAC1 = "MAC1", AUX = "AUX", E = "E";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            MAC1,
            document_symbol_kind::MACRO,
            range { { 0, 4 }, { 0, 4 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    AUX,
                    document_symbol_kind::MACH,
                    range { { 0, 4 }, { 0, 4 } },
                },
                document_symbol_item_s {
                    E,
                    document_symbol_kind::EQU,
                    range { { 0, 4 }, { 0, 4 } },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_macro_7)
{
    std::string opencode =
        R"(     MACRO
     MAC0
AUX0 LR   1,1
E0   EQU  1
     MAC1
     MEND

SEC0 CSECT
     MAC0
)";
    mock_parse_lib_provider mock({
        { "MAC1", R"(     MACRO
     MAC1
AUX1 LR   1,1
SEC1 DSECT
E1   EQU  1 
     MAC2
     MEND
)" },
        { "MAC2", R"(     MACRO
     MAC2
AUX2 LR   1,1
E2   EQU  1
     MEND
)" },
    });
    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string MAC0 = "MAC0", AUX0 = "AUX0", MAC1 = "MAC1", AUX1 = "AUX1", MAC2 = "MAC2", AUX2 = "AUX2", SEC0 = "SEC0",
                SEC1 = "SEC1", E0 = "E0", E1 = "E1", E2 = "E2";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            MAC0,
            document_symbol_kind::MACRO,
            range { { 8, 5 }, { 8, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC1,
                    document_symbol_kind::MACRO,
                    range { { 8, 5 }, { 8, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            MAC2,
                            document_symbol_kind::MACRO,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    E2,
                                    document_symbol_kind::EQU,
                                    range { { 8, 5 }, { 8, 5 } },
                                },
                            },
                        },
                        document_symbol_item_s {
                            E1,
                            document_symbol_kind::EQU,
                            range { { 8, 5 }, { 8, 5 } },
                        },
                        document_symbol_item_s {
                            SEC1,
                            document_symbol_kind::DUMMY,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    MAC2,
                                    document_symbol_kind::MACRO,
                                    range { { 8, 5 }, { 8, 5 } },
                                    document_symbol_list_s {
                                        document_symbol_item_s {
                                            AUX2,
                                            document_symbol_kind::MACH,
                                            range { { 8, 5 }, { 8, 5 } },
                                        },
                                    },
                                },
                            },
                        },
                    },
                },
                document_symbol_item_s {
                    E0,
                    document_symbol_kind::EQU,
                    range { { 8, 5 }, { 8, 5 } },
                },
            },
        },
        document_symbol_item_s {
            SEC0,
            document_symbol_kind::EXECUTABLE,
            range { { 7, 0 }, { 7, 0 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC0,
                    document_symbol_kind::MACRO,
                    range { { 8, 5 }, { 8, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            MAC1,
                            document_symbol_kind::MACRO,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    AUX1,
                                    document_symbol_kind::MACH,
                                    range { { 8, 5 }, { 8, 5 } },
                                },
                            },
                        },
                        document_symbol_item_s {
                            AUX0,
                            document_symbol_kind::MACH,
                            range { { 8, 5 }, { 8, 5 } },
                        },
                    },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_macro_8)
{
    std::string opencode =
        R"(     MACRO
     MAC0
AUX0 LR   1,1
E0   EQU  1
     MAC1
     MEND

SEC0 CSECT
     MAC0
)";
    mock_parse_lib_provider mock({
        { "MAC1", R"(     MACRO
     MAC1
SEC1 DSECT
AUX1 LR   1,1
E1   EQU  1 
     MAC2
     MEND
)" },
        { "MAC2", R"(     MACRO
     MAC2
SEC0 CSECT
AUX2 LR   1,1
E2   EQU  1
     MEND
)" },
    });
    analyzer a(opencode, analyzer_options(&mock));
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string MAC0 = "MAC0", AUX0 = "AUX0", MAC1 = "MAC1", AUX1 = "AUX1", MAC2 = "MAC2", AUX2 = "AUX2", SEC0 = "SEC0",
                SEC1 = "SEC1", E0 = "E0", E1 = "E1", E2 = "E2";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            MAC0,
            document_symbol_kind::MACRO,
            range { { 8, 5 }, { 8, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC1,
                    document_symbol_kind::MACRO,
                    range { { 8, 5 }, { 8, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            MAC2,
                            document_symbol_kind::MACRO,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    E2,
                                    document_symbol_kind::EQU,
                                    range { { 8, 5 }, { 8, 5 } },
                                },
                            },
                        },
                        document_symbol_item_s {
                            E1,
                            document_symbol_kind::EQU,
                            range { { 8, 5 }, { 8, 5 } },
                        },
                        document_symbol_item_s {
                            SEC1,
                            document_symbol_kind::DUMMY,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    AUX1,
                                    document_symbol_kind::MACH,
                                    range { { 8, 5 }, { 8, 5 } },
                                },
                            },
                        },
                    },
                },
                document_symbol_item_s {
                    E0,
                    document_symbol_kind::EQU,
                    range { { 8, 5 }, { 8, 5 } },
                },
            },
        },
        document_symbol_item_s {
            SEC0,
            document_symbol_kind::EXECUTABLE,
            range { { 7, 0 }, { 7, 0 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC0,
                    document_symbol_kind::MACRO,
                    range { { 8, 5 }, { 8, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            MAC1,
                            document_symbol_kind::MACRO,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    MAC2,
                                    document_symbol_kind::MACRO,
                                    range { { 8, 5 }, { 8, 5 } },
                                    document_symbol_list_s {
                                        document_symbol_item_s {
                                            AUX2,
                                            document_symbol_kind::MACH,
                                            range { { 8, 5 }, { 8, 5 } },
                                        },
                                    },
                                },
                            },
                        },
                        document_symbol_item_s {
                            AUX0,
                            document_symbol_kind::MACH,
                            range { { 8, 5 }, { 8, 5 } },
                        },
                    },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_macro_9)
{
    std::string opencode =
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
    mock_parse_lib_provider mock({
        { "MAC1", R"(     MACRO
     MAC1
SEC1 CSECT
AUX1 LR   1,1
E1   EQU  1 
     MEND
)" },
        { "MAC2", R"(     MACRO
     MAC2
AUX2 LR   1,1
E2   EQU  1
     MAC2
     MEND
)" },
        { "MAC3", R"(     MACRO
     MAC3
AUX3 LR   1,1
E3   EQU  1 
     MAC4
     MEND
)" },
        { "MAC4", R"(     MACRO
     MAC4
SEC1 CSECT
AUX4 LR   1,1
E4   EQU  1 
     MEND
)" },
    });
    analyzer a(opencode, analyzer_options(&mock));
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string MAC0 = "MAC0", AUX0 = "AUX0", MAC1 = "MAC1", AUX1 = "AUX1", MAC2 = "MAC2", AUX2 = "AUX2", SEC0 = "SEC0",
                SEC1 = "SEC1", E0 = "E0", E1 = "E1", E2 = "E2", MAC3 = "MAC3", AUX3 = "AUX3", E3 = "E3", MAC4 = "MAC4",
                AUX4 = "AUX4", E4 = "E4";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            MAC3,
            document_symbol_kind::MACRO,
            range { { 9, 5 }, { 9, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC4,
                    document_symbol_kind::MACRO,
                    range { { 9, 5 }, { 9, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            E4,
                            document_symbol_kind::EQU,
                            range { { 9, 5 }, { 9, 5 } },
                        },
                    },
                },
                document_symbol_item_s {
                    E3,
                    document_symbol_kind::EQU,
                    range { { 9, 5 }, { 9, 5 } },
                },
            },
        },
        document_symbol_item_s {
            MAC0,
            document_symbol_kind::MACRO,
            range { { 7, 5 }, { 7, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC1,
                    document_symbol_kind::MACRO,
                    range { { 7, 5 }, { 7, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            E1,
                            document_symbol_kind::EQU,
                            range { { 7, 5 }, { 7, 5 } },
                        },
                        document_symbol_item_s {
                            SEC1,
                            document_symbol_kind::EXECUTABLE,
                            range { { 7, 5 }, { 7, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    MAC4,
                                    document_symbol_kind::MACRO,
                                    range { { 9, 5 }, { 9, 5 } },
                                    document_symbol_list_s {
                                        document_symbol_item_s {
                                            AUX4,
                                            document_symbol_kind::MACH,
                                            range { { 9, 5 }, { 9, 5 } },
                                        },
                                    },
                                },
                                document_symbol_item_s {
                                    AUX1,
                                    document_symbol_kind::MACH,
                                    range { { 7, 5 }, { 7, 5 } },
                                },
                            },
                        },
                    },
                },
                document_symbol_item_s {
                    E0,
                    document_symbol_kind::EQU,
                    range { { 7, 5 }, { 7, 5 } },
                },
                document_symbol_item_s {
                    AUX0,
                    document_symbol_kind::MACH,
                    range { { 7, 5 }, { 7, 5 } },
                },
            },
        },
        document_symbol_item_s {
            SEC0,
            document_symbol_kind::DUMMY,
            range { { 8, 0 }, { 8, 0 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    MAC3,
                    document_symbol_kind::MACRO,
                    range { { 9, 5 }, { 9, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            AUX3,
                            document_symbol_kind::MACH,
                            range { { 9, 5 }, { 9, 5 } },
                        },
                    },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_copy)
{
    std::string opencode =
        R"(
    COPY COPYFILE1
)";
    mock_parse_lib_provider mock({ { "COPYFILE1", "" } });
    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    document_symbol_list_s expected = document_symbol_list_s {};
    EXPECT_EQ(outline, expected);
}

TEST(lsp_context_document_symbol, ord_copy_2)
{
    std::string opencode =
        R"(E0   EQU  1
AUX0 LR   1,1
     COPY COPYFILE1
)";
    mock_parse_lib_provider mock({
        { "COPYFILE1", R"(E1   EQU  1
AUX1 LR   1,1
)" },
    });
    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string E0 = "E0", AUX0 = "AUX0", E1 = "E1", AUX1 = "AUX1", COPYFILE1 = "COPYFILE1";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            COPYFILE1,
            document_symbol_kind::MACRO,
            range { { 2, 5 }, { 2, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    AUX1,
                    document_symbol_kind::MACH,
                    range { { 2, 5 }, { 2, 5 } },
                },
                document_symbol_item_s {
                    E1,
                    document_symbol_kind::EQU,
                    range { { 2, 5 }, { 2, 5 } },
                },
            },
        },
        document_symbol_item_s {
            AUX0,
            document_symbol_kind::MACH,
            range { { 1, 0 }, { 1, 0 } },
        },
        document_symbol_item_s {
            E0,
            document_symbol_kind::EQU,
            range { { 0, 0 }, { 0, 0 } },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}

TEST(lsp_context_document_symbol, ord_copy_3)
{
    std::string opencode =
        R"(     MACRO
     MAC0
EM0  EQU   1
AM0  LR    1,1
     COPY  COPYFILE1
     MEND

     MAC0
     MAC1
)";
    mock_parse_lib_provider mock({
        { "COPYFILE1", R"(EC1   EQU  1
SEC1  CSECT
AC1   LR   1,1
)" },
        { "COPYFILE2", R"(EC2  EQU  1
SEC2 DSECT
AC2  LR   1,1
     COPY COPYFILE3)" },
        { "COPYFILE3", R"(EC3   EQU  1
AC3   LR   1,1
)" },
        { "MAC1", R"(     MACRO
     MAC1
AM1  LR   1,1
EM1  EQU  1 
     COPY COPYFILE2
     MEND
)" },
    });
    analyzer a(opencode, analyzer_options { &mock });
    a.analyze();
    document_symbol_list_s outline = a.context().lsp_ctx->document_symbol("");
    std::string EM0 = "EM0", AM0 = "AM0", EM1 = "EM1", AM1 = "AM1", EC0 = "EC0", AC0 = "AC0", EC1 = "EC1", AC1 = "AC1",
                EC2 = "EC2", AC2 = "AC2", EC3 = "EC3", AC3 = "AC3", COPYFILE1 = "COPYFILE1", COPYFILE2 = "COPYFILE2",
                COPYFILE3 = "COPYFILE3", MAC0 = "MAC0", MAC1 = "MAC1", SEC1 = "SEC1", SEC2 = "SEC2";
    document_symbol_list_s expected = document_symbol_list_s {
        document_symbol_item_s {
            MAC1,
            document_symbol_kind::MACRO,
            range { { 8, 5 }, { 8, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    COPYFILE2,
                    document_symbol_kind::MACRO,
                    range { { 8, 5 }, { 8, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            EC2,
                            document_symbol_kind::EQU,
                            range { { 8, 5 }, { 8, 5 } },
                        },
                        document_symbol_item_s {
                            COPYFILE3,
                            document_symbol_kind::MACRO,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    EC3,
                                    document_symbol_kind::EQU,
                                    range { { 8, 5 }, { 8, 5 } },
                                },
                            },
                        },
                        document_symbol_item_s {
                            SEC2,
                            document_symbol_kind::DUMMY,
                            range { { 8, 5 }, { 8, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    COPYFILE3,
                                    document_symbol_kind::MACRO,
                                    range { { 8, 5 }, { 8, 5 } },
                                    document_symbol_list_s {
                                        document_symbol_item_s {
                                            AC3,
                                            document_symbol_kind::MACH,
                                            range { { 8, 5 }, { 8, 5 } },
                                        },
                                    },
                                },
                                document_symbol_item_s {
                                    AC2,
                                    document_symbol_kind::MACH,
                                    range { { 8, 5 }, { 8, 5 } },
                                },
                            },
                        },
                    },
                },
                document_symbol_item_s {
                    EM1,
                    document_symbol_kind::EQU,
                    range { { 8, 5 }, { 8, 5 } },
                },
            },
        },
        document_symbol_item_s {
            MAC0,
            document_symbol_kind::MACRO,
            range { { 7, 5 }, { 7, 5 } },
            document_symbol_list_s {
                document_symbol_item_s {
                    COPYFILE1,
                    document_symbol_kind::MACRO,
                    range { { 7, 5 }, { 7, 5 } },
                    document_symbol_list_s {
                        document_symbol_item_s {
                            EC1,
                            document_symbol_kind::EQU,
                            range { { 7, 5 }, { 7, 5 } },
                        },
                        document_symbol_item_s {
                            SEC1,
                            document_symbol_kind::EXECUTABLE,
                            range { { 7, 5 }, { 7, 5 } },
                            document_symbol_list_s {
                                document_symbol_item_s {
                                    AM1,
                                    document_symbol_kind::MACH,
                                    range { { 8, 5 }, { 8, 5 } },
                                },
                                document_symbol_item_s {
                                    AC1,
                                    document_symbol_kind::MACH,
                                    range { { 7, 5 }, { 7, 5 } },
                                },
                            },
                        },
                    },
                },
                document_symbol_item_s {
                    AM0,
                    document_symbol_kind::MACH,
                    range { { 7, 5 }, { 7, 5 } },
                },
                document_symbol_item_s {
                    EM0,
                    document_symbol_kind::EQU,
                    range { { 7, 5 }, { 7, 5 } },
                },
            },
        },
    };
    EXPECT_TRUE(is_permutation_with_permutations(outline, expected));
}
