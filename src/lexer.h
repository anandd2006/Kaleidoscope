#pragma once
#include <istream>
#include <string>

// Shared token details, matching the tutorial's names.
// They are declared here and defined once in lexer.cpp.
extern std::string IdentifierStr;
extern double NumVal;

void InitializeLexer(std::istream &input);
int gettok();
