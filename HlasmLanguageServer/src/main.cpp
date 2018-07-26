#include <iostream>
#include <string>

#include "shared/HlasmParserLibrary.h"

int main() {
	std::string input;
	std::cin >> input;
	HlasmParserLibrary::HlasmParserLibrary::parse(std::move(input));
	std::cin.get();
	return 0;
}