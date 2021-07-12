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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TEST_COPY_MOCK_H
#define HLASMPLUGIN_PARSERLIBRARY_TEST_COPY_MOCK_H

#include "analyzer.h"
#include "../mock_parse_lib_provider.h"

namespace hlasm_plugin::parser_library {

class copy_mock
{
public:
    static mock_parse_lib_provider create()
    {
        return mock_parse_lib_provider { { "COPYR", content_COPYR },
            { "COPYF", content_COPYF },
            { "COPYD", content_COPYD },
            { "COPYREC", content_COPYREC },
            { "COPYU", content_COPYU },
            { "COPYL", content_COPYL },
            { "COPYN", content_COPYN },
            { "MAC", content_MAC },
            { "COPYM", content_COPYM },
            { "COPYJ", content_COPYJ },
            { "COPYJF", content_COPYJF },
            { "COPYND1", content_COPYND1 },
            { "COPYND2", content_COPYND2 },
            { "COPYBM", content_COPYBM },
            { "EMPTY", content_EMPTY },
            { "COPYEMPTY", content_COPYEMPTY } };
    }

private:
    static inline const std::string content_COPYR =
        R"(   
 LR 1,1
 MACRO
 M1
 LR 1,1
 
 MACRO
 M2
 LR 2,2
 MEND
 AGO .A
.A ANOP
 MEND

&VARX SETA &VARX+1
.A ANOP
.B ANOP
&VAR SETA &VAR+1
)";
    static inline const std::string content_COPYF =
        R"(  
 LR 1,1
&VARX SETA &VARX+1
 COPY COPYR
&VAR SETA &VAR+1
.C ANOP
)";

    static inline const std::string content_COPYD =
        R"(  

 LR 1,
)";

    static inline const std::string content_COPYREC =
        R"(  
 ANOP
 COPY COPYREC
 ANOP
)";

    static inline const std::string content_COPYU =
        R"(  
 ANOP
 MACRO
 M
 MEND
 MEND
 ANOP
)";

    static inline const std::string content_COPYL =
        R"(  
 LR 1,1
.A ANOP
&VARX SETA &VARX+1
 AGO .X
&VAR SETA &VAR+1
.A ANOP
.C ANOP
)";

    static inline const std::string content_COPYN =
        R"( 
 MAC
)";

    static inline const std::string content_MAC =
        R"( MACRO
 MAC
 LR 1,1
 COPY COPYM
 MEND
)";

    static inline const std::string content_COPYM =
        R"(
.A ANOP
 GBLA &X
&X SETA 4
)";

    static inline const std::string content_COPYJ =
        R"(
 AGO .X
 ;%
.X ANOP
)";
    static inline const std::string content_COPYJF =
        R"(
 AGO .X
 LR
)";

    static inline const std::string content_COPYND1 =
        R"(
 COPY COPYND2
)";

    static inline const std::string content_COPYND2 =
        R"(



 LR 1,)";

    static inline const std::string content_COPYBM =
        R"( 
 MACRO
 M
 LR 1
 MEND
)";
    static inline const std::string content_EMPTY = "";
    static inline const std::string content_COPYEMPTY = " COPY EMPTY";
};

} // namespace hlasm_plugin::parser_library

#endif
