# Kaleidoscope: a compiler learning project

I started this project to learn how a compiler works by following the
[official LLVM Kaleidoscope tutorial](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html).
It follows Chapters 1-3 for the lexer, parser/AST, and LLVM IR, plus the
if/then/else part of Chapter 5.

This is a tutorial-based learning project, developed with help from OpenAI
ChatGPT/Codex for explanations, implementation, debugging, and build setup.
The working baseline was completed with substantial AI assistance. I am using
it to understand the compiler and build additional features myself.

For a detailed explanation in simple English, read the [compiler study notes](NOTES.md).

## What I am learning

- How a lexer turns source characters into tokens.
- How a parser handles expressions and operator precedence.
- How AST nodes represent numbers, variables, operations, and functions.
- How LLVM's C++ API turns those nodes into LLVM IR.
- How to build a C++ project with CMake and check its behavior with tests.

## Language

All values are double-precision numbers. Supports numeric literals, variables
that refer to function parameters, parentheses, +, -, *, /, <, function
definitions, extern declarations, calls, unary minus, underscore identifiers,
if/then/else, and # comments. Operators associate
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

## Execute Fibonacci and print the answer

Compiling produces LLVM IR; it does not print the calculated result.
These PowerShell commands compile the example, add a separate IR entry point
that prints the result, and execute it with LLVM's `lli`.

Run from the project directory after building. Adjust the `lli.exe` path if
your LLVM installation is elsewhere.

```powershell
.\build-x64\Release\toycc.exe examples\fib.toy -o build-x64\fib.ll
Copy-Item build-x64\fib.ll build-x64\run-fib.ll

@'

@fmt = private constant [4 x i8] c"%g\0A\00"
declare i32 @printf(ptr, ...)

define i32 @main() {
entry:
  %answer = call double @__anon_expr0()
  call i32 (ptr, ...) @printf(ptr @fmt, double %answer)
  ret i32 0
}
'@ | Add-Content build-x64\run-fib.ll

& ".\tools\clang+llvm-20.1.8-x86_64-pc-windows-msvc\bin\lli.exe" build-x64\run-fib.ll
```

Expected output:

```text
55
```

The wrapper calls the first standalone expression in the file, currently
`fib(10)`. Changing that expression to `fib(12)`, recompiling, and recreating
the runnable copy will print 144. No Fibonacci argument is hardcoded in this
wrapper. With multiple standalone expressions, it still calls only the first
wrapper, `__anon_expr0`.

Always copy the freshly compiled IR before appending the entry point.
Appending it twice causes duplicate-definition errors.
Execution uses an external LLVM tool; a built-in run option is still a future feature.

## Files and places to extend

- src/token.h: token codes from Chapter 1.
- src/lexer.h / lexer.cpp: character scanning; add keywords here.
- src/ast.h: expression, prototype, and function nodes.
- src/parser.h / parser.cpp: grammar and precedence; add new syntax here.
- src/codegen.h / codegen.cpp: LLVM context, module, builder, and codegen methods.
- src/main.cpp: command-line options, token display, and compilation loop.
- examples/: arithmetic, external calls, and recursive Fibonacci examples.
- tests/smoke.cmake: valid programs, precedence, calls, and rejected input.
- tests/conditionals.cmake: conditional IR, syntax errors, and runtime results.

## Features I added while learning

After working through the baseline, I made these changes with step-by-step
explanations and guidance from ChatGPT/Codex:

### Underscores in identifiers

I updated the lexer so an identifier can start with an underscore and contain
underscores after its first character. Names such as `my_func`, `_value2`, and
`my_value` are now accepted.

```text
def my_func(_value2) _value2 + 2;
my_func(3);
```

This helped me understand how the lexer checks the first character of a name
and decides which following characters belong to the same token.

### Unary minus

I added `ParseUnaryMinusExpr()` and connected it to the `'-'` case in
`ParsePrimary()`. It skips the minus sign, parses its operand, and represents
`-x` as `0 - x` using the existing `BinaryExprAST` subtraction node.

```text
-5 + 2;       # -3
2 * -3;       # -6
-(2 + 3);     # -5
```

Parsing the operand with `ParsePrimary()` keeps unary minus more tightly bound
than multiplication or addition. Existing AST and LLVM subtraction code handle
the result, so this approach did not need a new AST class or codegen method.

This helped me understand token consumption, recursive parsing, operator
precedence, and transferring AST ownership with `std::move`.

### If / then / else and Fibonacci

I followed Chapter 5 to add conditional expressions. With guidance, I added
the keyword tokens, lexer checks, an `IfExprAST` node, and `ParseIfExpr()`.
ChatGPT/Codex helped complete and align the LLVM code generation with the tutorial.

```text
if 2 < 3 then 10 else 20;
```

The compiler generates a conditional branch, a block for each path, and a merge
block. A PHI node chooses the result from the path that ran. Both branches are
compiled, but only the selected branch executes. Zero is false; nonzero values
are true, except NaN, which is treated as false.

This helped me understand basic blocks, branch instructions, PHI nodes, nested
conditionals, and the difference between generating code and executing it.

The example in [examples/fib.toy](examples/fib.toy) uses a base case and recursive calls:

```text
def fib(x)
  if x < 3 then
    1
  else
    fib(x-1) + fib(x-2);

fib(10);
```

For positive integer inputs, this example returns 1 for the first two Fibonacci
numbers. The expected result of `fib(10)` is **55**. This simple recursive
version repeats work, so use small inputs while experimenting.

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
JIT execution, object-file emission, optimization passes, loops,
or mutable variables. It is not an implementation of every chapter.

Possible next exercises are more comparisons and source locations in error
messages.

One follow-up for underscore support remains: internal expression wrappers still
use names such as `__anon_expr0`. A user can now write the same name, so those
internal names need a separate naming scheme to prevent collisions.

## Acknowledgements

- The **LLVM Project and the authors and contributors of the Kaleidoscope
  tutorial** for the language, compiler design, examples, and reference
  implementation on which this project is based:
  [Chapter 1: Lexer](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl01.html),
  [Chapter 2: Parser and AST](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html),
  [Chapter 3: LLVM IR](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html),
  and [Chapter 5: Control flow](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl05.html).
- **LLVM** for the libraries and tools used to generate, verify, and execute IR.
- **OpenAI ChatGPT/Codex** for guided explanations and substantial assistance
  implementing the baseline, troubleshooting the toolchain, and writing tests.

## Verified locally

Built with Visual Studio 2022 x64 and LLVM 20.1.8. CTest checks arithmetic,
precedence, associativity, calls, declarations, source-file protection, and
invalid programs. When lli and llvm-as are available beside LLVM, the suite
also assembles and executes generated IR, checking arithmetic, nested conditions,
branch selection, and fib(20) = 6765.

