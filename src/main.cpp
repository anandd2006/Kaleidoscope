#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

static void PrintTokens() {
    while (true) {
        int token = gettok();
        switch (token) {
        case tok_eof: std::cout << "eof\n"; return;
        case tok_def: std::cout << "def\n"; break;
        case tok_extern: std::cout << "extern\n"; break;
        case tok_identifier: std::cout << "identifier " << IdentifierStr << '\n'; break;
        case tok_number: std::cout << "number " << NumVal << '\n'; break;
        case tok_if: std::cout<<"if\n"; break;
        case tok_then: std::cout<<"then\n";break;
        case tok_else: std::cout<<"else\n";break;
        default: std::cout << "'" << static_cast<char>(token) << "'\n"; break;
        }
    }
}

int main(int argc, char **argv) {
    try {
        std::string inputPath, outputPath;
        bool tokens = false;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help") {
                std::cout << "Usage: toycc [--tokens] [input.toy] [-o output.ll]\n"
                             "Without an input file, reads standard input.\n";
                return 0;
            }
            if (arg == "--tokens") { tokens = true; continue; }
            if (arg == "-o") {
                if (++i == argc) throw std::runtime_error("Missing filename after -o");
                outputPath = argv[i];
                continue;
            }
            if (!arg.empty() && arg[0] == '-') throw std::runtime_error("Unknown option: " + arg);
            if (!inputPath.empty()) throw std::runtime_error("Only one input file is supported");
            inputPath = arg;
        }
        if (tokens && !outputPath.empty())
            throw std::runtime_error("--tokens writes to stdout; do not combine with -o");
        // Reject the same file even when the paths use different spellings.
        if (!inputPath.empty() && !outputPath.empty()) {
            auto source = std::filesystem::weakly_canonical(inputPath);
            auto output = std::filesystem::weakly_canonical(outputPath);
            if (source == output ||
                (std::filesystem::exists(output) && std::filesystem::equivalent(source, output)))
                throw std::runtime_error("Input and output must be different files");
        }
        std::ifstream file;
        std::istream *input = &std::cin;
        if (!inputPath.empty()) {
            file.open(inputPath);
            if (!file) throw std::runtime_error("Cannot open input: " + inputPath);
            input = &file;
        }
        InitializeLexer(*input);
        if (tokens) { PrintTokens(); return 0; }

        InitializeParser();
        InitializeModule(inputPath.empty() ? "stdin" : inputPath);
        getNextToken();
        while (CurTok != tok_eof) {
            switch (CurTok) {
            case ';': getNextToken(); break;
            case tok_def: ParseDefinition()->codegen(); break;
            case tok_extern: ParseExtern()->codegen(); break;
            default: ParseTopLevelExpr()->codegen(); break;
            }
        }
        // Emit only after the entire input has compiled successfully.
        WriteModule(outputPath);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
