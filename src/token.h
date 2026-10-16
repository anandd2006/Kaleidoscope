#pragma once

// Like Chapter 1: negative values identify special tokens.
// Operators and punctuation are returned as character codes.
enum Token {
    tok_eof = -1,
    tok_def = -2,
    tok_extern = -3,
    tok_identifier = -4,
    tok_number = -5
};
