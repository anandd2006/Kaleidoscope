# Kaleidoscope: a compiler learning project

I started this project to learn how a compiler works by following the
[official LLVM Kaleidoscope tutorial](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html).
It follows the lexer, parser/AST, and LLVM IR stages covered in Chapters 1-3.

This is a tutorial-based learning project, developed with help from OpenAI
ChatGPT/Codex for explanations, implementation, debugging, and build setup.
The working baseline was completed with substantial AI assistance. I am using
it to understand the compiler and build additional features myself.

## What I am learning

- How a lexer turns source characters into tokens.
- How a parser handles expressions and operator precedence.
- How AST nodes represent numbers, variables, operations, and functions.
- How LLVM's C++ API turns those nodes into LLVM IR.
- How to build a C++ project with CMake and check its behavior with tests.

## Language

All values are double-precision numbers. Supports numeric literals, variables
that refer to function parameters, parentheses, +, -, *, /, <, function
definitions, extern declarations, calls, and # comments. Operators associate
left to right; multiplication/division bind before addition/subtraction, and
comparison has the lowest precedence. Semicolons can separate top-level items.

```text
extern sqrt(x);
def add(a b) a + b;
def square(x) x * x;
add(2, square(3));
```

Definitions/prototypes separate parameter names with spaces. Calls separate
argument expressions with commas. Declare a function before calling it;
self-recursion works. Standalone expressions become zero-argument functions
named __anon_expr0, __anon_expr1, etc. The compiler emits IR; it does not
automatically execute those expressions or print their numeric results.

## Build

Install a C++17 compiler, CMake, and the LLVM development package (headers,
libraries, and LLVMConfig.cmake). A command-line-only LLVM install may omit
development files. On Windows use an x64 development package and an x64
Visual Studio C++ toolchain.

PowerShell (from the project directory). My local setup uses LLVM 20.1.8
package in tools/clang+llvm-20.1.8-x86_64-pc-windows-msvc, automatically
selected by CMake when present. This package is not included in the repository.
The tools directory is local build infrastructure and is ignored by Git.

For another installation, append -DLLVM_DIR=C:/your/llvm/lib/cmake/llvm to
the configure command.

```powershell
cmake -S . -B build-x64 -A x64
cmake --build build-x64 --config Release
ctest --test-dir build-x64 -C Release --output-on-failure
.\build-x64\Release\toycc.exe examples/add.toy -o build-x64/add.ll
Get-Content build-x64/add.ll
"2 + 3 * 4" | .\build-x64\Release\toycc.exe --tokens
```

Linux/macOS (set LLVM_DIR to your installation's CMake package directory):

```sh
cmake -S . -B build -DLLVM_DIR="$(llvm-config --cmakedir)"
cmake --build build
ctest --test-dir build --output-on-failure
./build/toycc examples/add.toy -o build/add.ll
echo '2 + 3 * 4' | ./build/toycc --tokens
```

Without a filename, toycc reads standard input until EOF. Use --help for usage.
Errors go to stderr and return exit code 1. Successful compilation returns 0.
IR is emitted only after the whole input succeeds.

## Files and places to extend

- src/token.h: token codes from Chapter 1.
- src/lexer.h / lexer.cpp: character scanning; add keywords here.
- src/ast.h: expression, prototype, and function nodes.
- src/parser.h / parser.cpp: grammar and precedence; add new syntax here.
- src/codegen.h / codegen.cpp: LLVM context, module, builder, and codegen methods.
- src/main.cpp: command-line options, token display, and compilation loop.
- examples/: sample programs.
- tests/smoke.cmake: valid programs, precedence, calls, and rejected input.

## Changes and additions to the tutorial baseline

These are changes in this repository, developed with AI assistance, rather
than claims that the underlying compiler design is original:

- Split the tutorial implementation into lexer, parser, AST, code-generation,
  and main files under src/.
- Added CMake build configuration, sample .toy programs, and regression checks.
- Replaced getchar input with a stream so the compiler accepts files or stdin.
- Added --tokens to inspect lexer output and -o to save generated LLVM IR.
- Added division with the same precedence as multiplication.
- Rejected malformed numbers such as 1.2.3 and a lone decimal point.
- Added checks for duplicate parameters, conflicting declarations, redefinitions,
  unknown names, and incorrect function-call argument counts.
- Used exceptions to stop compilation on errors, with diagnostics on stderr.
- Gave standalone expressions unique wrapper names so several can share a module.
- Prevented an output path from overwriting the input source file.
- Used assignments inside constructors and explanatory comments to make the code
  easier for me to study.

## Current scope and next steps

This project generates LLVM IR. It does not yet include a built-in run mode,
JIT execution, object-file emission, optimization passes, conditionals, loops,
mutable variables, or unary minus. It is not an implementation of every chapter.

Possible next exercises are underscore support in identifiers, unary minus,
more comparisons, and source locations in error messages. Underscore support
has been discussed but is not implemented in the current saved lexer.

## Acknowledgements

- The **LLVM Project and the authors and contributors of the Kaleidoscope
  tutorial** for the language, compiler design, examples, and reference
  implementation on which this project is based:
  [Chapter 1: Lexer](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl01.html),
  [Chapter 2: Parser and AST](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html),
  and [Chapter 3: LLVM IR](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html).
- **LLVM** for the libraries and tools used to generate, verify, and execute IR.
- **OpenAI ChatGPT/Codex** for guided explanations and substantial assistance
  implementing the baseline, troubleshooting the toolchain, and writing tests.

## Verified locally

Built with Visual Studio 2022 x64 and LLVM 20.1.8. CTest checks arithmetic,
precedence, associativity, calls, declarations, source-file protection, and
invalid programs. When lli and llvm-as are available beside LLVM, the suite
also assembles generated IR and executes a program that checks the result 14.

