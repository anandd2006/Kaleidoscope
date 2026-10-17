#include "lexer.h"
#include "token.h"
#include <cctype>
#include <cstdlib>
#include <stdexcept>

std::string IdentifierStr;
double NumVal = 0;

namespace {
std::istream *Input = nullptr;
int LastChar = ' ';

// EOF is not a character. Cast ordinary characters for cctype functions.
bool IsSpace(int c) { return c != std::char_traits<char>::eof() && std::isspace(static_cast<unsigned char>(c)); }
bool IsAlpha(int c) { return c != std::char_traits<char>::eof() && std::isalpha(static_cast<unsigned char>(c)); }
bool IsDigit(int c) { return c != std::char_traits<char>::eof() && std::isdigit(static_cast<unsigned char>(c)); }
bool IsAlnum(int c) { return IsAlpha(c) || IsDigit(c); }
}

void InitializeLexer(std::istream &input) {
    Input = &input;
    LastChar = ' ';
    IdentifierStr.clear();
    NumVal = 0;
}

int gettok() {
    // Loop instead of recursion so thousands of comment lines are safe.
    while (true) {
        while (IsSpace(LastChar))
            LastChar = Input->get();

        if (LastChar != '#')
            break;

        do {
            LastChar = Input->get();
        } while (LastChar != std::char_traits<char>::eof() && LastChar != '\n' && LastChar != '\r');
    }

    if (IsAlpha(LastChar)||LastChar=='_') {
        IdentifierStr.clear();
        do {
            IdentifierStr += static_cast<char>(LastChar);
            LastChar = Input->get();
        } while (IsAlnum(LastChar)||LastChar=='_');

        if (IdentifierStr == "def") return tok_def;
        if (IdentifierStr == "extern") return tok_extern;
        if (IdentifierStr=="if")return tok_if;
        if (IdentifierStr=="then")return tok_then;
        if (IdentifierStr=="else")return tok_else;
        return tok_identifier;
    }

    if (IsDigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += static_cast<char>(LastChar);
            LastChar = Input->get();
        } while (IsDigit(LastChar) || LastChar == '.');

        // Unlike the tutorial's minimal scanner, reject 1.2.3 and ".".
        char *end = nullptr;
        NumVal = std::strtod(NumStr.c_str(), &end);
        if (end == NumStr.c_str() || *end != '\0')
            throw std::runtime_error("Invalid number: " + NumStr);
        return tok_number;
    }

    if (LastChar == std::char_traits<char>::eof())
        return tok_eof;

    int ThisChar = LastChar;
    LastChar = Input->get();
    return ThisChar;
}
