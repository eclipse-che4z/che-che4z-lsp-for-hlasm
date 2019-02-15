#pragma once
#include "common_testing.h"
#include "../src/ebcdic_encoding.h"

TEST(input_source, utf8conv)
{
	std::string u8;

	u8.insert(u8.end(), (unsigned char)0xf0);
	u8.insert(u8.end(), (unsigned char)0x90);
	u8.insert(u8.end(), (unsigned char)0x80);
	u8.insert(u8.end(), (unsigned char)0x80);

	hlasm_plugin::parser_library::input_source input1(u8);

	EXPECT_EQ(u8, input1.getText({ (ssize_t)0,(ssize_t)0 }));

	u8.insert(u8.end(), (unsigned char)0xEA);
	u8.insert(u8.end(), (unsigned char)0x84);
	u8.insert(u8.end(), (unsigned char)0xA3);

	hlasm_plugin::parser_library::input_source input2(u8);

	EXPECT_EQ(u8, input2.getText({ (ssize_t)0,(ssize_t)1 }));

	u8.insert(u8.end(), (unsigned char)0xC5);
	u8.insert(u8.end(), (unsigned char)0x80);

	hlasm_plugin::parser_library::input_source input3(u8);

	EXPECT_EQ(u8, input3.getText({ (ssize_t)0,(ssize_t)2 }));

	u8.insert(u8.end(), (unsigned char)0x41);

	hlasm_plugin::parser_library::input_source input4(u8);

	EXPECT_EQ(u8, input4.getText({ (ssize_t)0,(ssize_t)3 }));
}

TEST(ebcdic_encoding, unicode)
{
	std::string u8;

	u8.insert(u8.end(), (unsigned char)0xf0);
	u8.insert(u8.end(), (unsigned char)0x90);
	u8.insert(u8.end(), (unsigned char)0x80);
	u8.insert(u8.end(), (unsigned char)0x80);

	u8.insert(u8.end(), (unsigned char)0xEA);
	u8.insert(u8.end(), (unsigned char)0x84);
	u8.insert(u8.end(), (unsigned char)0xA3);

	u8.insert(u8.end(), (unsigned char)0xC5);
	u8.insert(u8.end(), (unsigned char)0x80);

	u8.insert(u8.end(), (unsigned char)0x41);

	//ä
	u8.insert(u8.end(), (unsigned char)0xC3); 
	u8.insert(u8.end(), (unsigned char)0xA4);

	auto begin = u8.c_str();

	EXPECT_EQ(hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(begin), hlasm_plugin::parser_library::ebcdic_encoding::SUB);

	EXPECT_EQ(begin, u8.c_str() + 3);


	EXPECT_EQ(hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(++begin), hlasm_plugin::parser_library::ebcdic_encoding::SUB);

	EXPECT_EQ(begin, u8.c_str() + 4 + 2);


	EXPECT_EQ(hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(++begin), hlasm_plugin::parser_library::ebcdic_encoding::SUB);

	EXPECT_EQ(begin, u8.c_str() + 4 + 3 + 1);


	EXPECT_EQ(hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(++begin), 0x41);

	EXPECT_EQ(begin, u8.c_str() + 4 + 3 + 2);


	EXPECT_EQ(hlasm_plugin::parser_library::ebcdic_encoding::to_pseudoascii(++begin), (unsigned char)0xE4);

	EXPECT_EQ(begin, u8.c_str() + 4 + 3 + 2 + 1 + 1);
}
