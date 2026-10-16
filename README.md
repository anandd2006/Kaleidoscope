# Toy compiler

A small C++17 compiler following LLVM's Kaleidoscope tutorial through Chapter 3:
source text -> lexer -> parser / AST -> LLVM IR. The familiar names (gettok,
CurTok, ParseExpression, ParseBinOpRHS, codegen) are kept, with explanatory comments.

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

PowerShell (from C:/compiler). This machine has a compatible LLVM 20.1.8
package in tools/clang+llvm-20.1.8-x86_64-pc-windows-msvc, automatically
selected by CMake. Its existing system LLVM is ARM64 and cannot run here.
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

## Differences from the tutorial

The same concepts are split into files instead of a single source file.
An input stream replaces getchar so files and standard input both work.
Errors use exceptions to stop the batch compilation at the first failure.
Malformed numbers, duplicate parameters, mismatched declarations, and function
redefinitions are rejected. Division and --tokens are small additions.
Top-level expressions have unique names so several can coexist in one module.
Constructor bodies use assignments for readability.

This baseline intentionally leaves later-chapter features for you to add:
unary minus (write 0-x for now), if/then/else, loops, mutable variables, custom
operators, optimization passes, JIT execution, and object-file emission.
It is not a complete implementation of every tutorial chapter.

Tutorial references:
- https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl01.html
- https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html
- https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html

Your original source files are preserved in the backup-before-completion folder.

## Verified locally

Built with Visual Studio 2022 x64 and LLVM 20.1.8. CTest checks arithmetic,
precedence, associativity, calls, declarations, source-file protection, and
invalid programs. When lli and llvm-as are available beside LLVM, the suite
also assembles generated IR and executes a program that checks the result 14.

Try adding unary minus first: teach ParsePrimary to recognize '-' before an
expression, then represent it as 0 minus that expression. Add a regression
case to tests/smoke.cmake before changing the parser.
