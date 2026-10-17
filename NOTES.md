# Kaleidoscope compiler: study notes

These notes explain the current code in this repository in simple English.
Read them beside the source files. You do not need to learn everything at once.

The project follows LLVM's Kaleidoscope tutorial through Chapters 1-3, with
some changes and extra checks. The baseline was completed with AI assistance.
These notes describe the saved implementation, not every later tutorial feature.

## Contents

1. [What this project does](#1 -what-this-project-does)
2. [The language you can write](#2-the-language-you-can-write)
3. [The files and how they connect](#3-the-files-and-how-they-connect)
4. [C++ ideas used in this project](#4-c-ideas-used-in-this-project)
5. [Tokens: token.h](#5-tokens-tokenh)
6. [Reading characters: lexer.h and lexer.cpp](#6-reading-characters-lexerh-and-lexercpp)
7. [The program tree: ast.h](#7-the-program-tree-asth)
8. [Reading tokens: parser.h and parser.cpp](#8-reading-tokens-parserh-and-parsercpp)
9. [Operator precedence, slowly](#9-operator-precedence-slowly)
10. [Functions, declarations, and calls](#10-functions-declarations-and-calls)
11. [Making LLVM IR: codegen.h and codegen.cpp](#11-making-llvm-ir-codegenh-and-codegencpp)
12. [Reading LLVM IR](#12-reading-llvm-ir)
13. [The controller: main.cpp](#13-the-controller-maincpp)
14. [Building: CMakeLists.txt](#14-building-cmakeliststxt)
15. [Tests and examples](#15-tests-and-examples)
16. [How to build, compile, and execute](#16-how-to-build-compile-and-execute)
17. [Two full worked examples](#17-two-full-worked-examples)
18. [Errors, limits, and differences from the tutorial](#18-errors-limits-and-differences-from-the-tutorial)
19. [Where to add features](#19-where-to-add-features)
20. [Practice questions and glossary](#20-practice-questions-and-glossary)
21. [References and acknowledgements](#21-references-and-acknowledgements)

## 1. What this project does

A compiler translates a program into another form.

Our input is a small language:

~~~text
def add(a b) a + b;
add(2, 3);
~~~

Our output is LLVM IR. IR means intermediate representation: a form of code
that LLVM understands.

The main path is:

~~~text
Source characters
       |
       v
Lexer: recognizes tokens
       |
       v
Parser: checks structure and builds an AST
       |
       v
Code generator: creates LLVM IR
       |
       v
LLVM IR printed on screen or saved as a .ll file
~~~

These stages answer different questions:

| Stage | Question |
| --- | --- |
| Lexer | Which words, numbers, and symbols are present? |
| Parser | How do those pieces fit together? |
| AST | What structure represents that expression? |
| Code generator | Which LLVM values and instructions represent that structure? |
| Execution tool | What happens when the generated program runs? |

The compiler currently stops after generating IR. We can use LLVM's lli tool
to execute suitable IR, but execution is not built into toycc.

Three different activities appear in this project:

- Build the compiler: turn our C++ files into toycc.exe.
- Compile a toy program: run toycc.exe to produce LLVM IR.
- Execute the compiled program: run suitable IR with lli.

A successful build does not mean a toy program has been executed.

## 2. The language you can write

### Numbers

~~~text
2
23.5
.5
2.
~~~

All language values are double-precision floating-point numbers.
There is no separate integer type. The number 2 is represented as a double too.

Scientific notation such as 1e3 is not supported as one number token.
A lone dot and numbers such as 1.2.3 are rejected.

### Expressions and operators

~~~text
2 + 3
8 / 2
(2 + 3) * 4
2 < 3
~~~

Supported binary operators are +, -, *, /, and <.
Binary means the operator has two sides.

| Operators | Precedence | Meaning |
| --- | --- | --- |
| < | 10 | Comparison |
| + and - | 20 | Addition and subtraction |
| * and / | 40 | Multiplication and division |

Higher numbers bind more tightly. The numbers 10, 20, and 40 are our choices;
their relative order matters.

Operators of the same precedence group from left to right.

~~~text
20 - 5 - 3      means (20 - 5) - 3
8 / 2 * 4      means (8 / 2) * 4
~~~

For ordinary numeric values, < produces 1.0 when true and 0.0 when false.
It uses floating-point comparison semantics; it does not produce a separate
boolean language type.

Unary minus is not implemented. For now, write 0 - 5 instead of -5.

### Names

The current lexer accepts a letter followed by letters or digits.

~~~text
x
value2
add
~~~

Names are case-sensitive: value and Value are different.

Underscores are not yet supported in the saved lexer.
Digits cannot start an identifier. Non-ASCII name handling is not a designed
feature of this small language.

### Definitions, declarations, and calls

~~~text
def add(a b) a + b;
extern sqrt(x);
add(2, 3);
~~~

A definition gives a function its body.
An extern declaration gives a function's name and parameters without a body.
A call asks to run a function with supplied arguments.

Parameter names in definitions and extern declarations use spaces.
Arguments in calls use commas.

~~~text
def add(a b) a + b;   # Correct definition
add(2, 3);           # Correct call
~~~

The body is one expression. There are no statement blocks or explicit return
keywords. The body's value becomes the function's return value.

Variables currently mean function parameters. There are no local variable
declarations or assignments.

Declare or define a function before calling it. A function can call itself
because code generation creates its declaration before generating its body.

### Comments and semicolons

~~~text
# Everything from this symbol to the line ending is a comment.
2 + 3;
~~~

Semicolons can separate top-level items. The main loop skips them.
A newline is whitespace; it is not a special end-of-statement token.

## 3. The files and how they connect

| File | Job |
| --- | --- |
| src/token.h | Defines special token codes |
| src/lexer.h | Declares the lexer functions and shared token details |
| src/lexer.cpp | Reads characters and recognizes tokens |
| src/ast.h | Defines the expression and function node classes |
| src/parser.h | Declares the parser's public entry points |
| src/parser.cpp | Reads tokens and builds AST nodes |
| src/codegen.h | Declares module setup and output functions |
| src/codegen.cpp | Implements AST codegen methods using LLVM |
| src/main.cpp | Reads options and connects the stages |
| CMakeLists.txt | Describes how to build and test the compiler |
| examples/add.toy | Demonstrates functions and arithmetic |
| examples/functions.toy | Demonstrates externs, calls, and comparison |
| tests/smoke.cmake | Checks successful and unsuccessful programs |
| .gitignore | Keeps build outputs and downloaded tools out of Git |
| README.md | Explains project purpose, features, and commands |

A header (.h) usually tells other files what names and functions exist.
A source file (.cpp) usually contains their implementation.

For example, lexer.h declares gettok(). lexer.cpp defines its body.

The compiler builds each .cpp file separately into an object file. The linker
then joins those object files and the required LLVM libraries into toycc.exe.
This is why a function used across files must have the correct declaration
and a definition that other files can link to.

## 4. C++ ideas used in this project

### Includes and header guards

~~~cpp
#include <string>
#include "token.h"
#pragma once
~~~

Angle brackets normally name a library header.
Quotes normally name one of our project headers.
Pragma once prevents repeated processing of the same header within one C++
translation unit. It does not mean the header is read only once for the
entire project.

### Classes, objects, and members

A class describes a kind of object. A member is data or a function inside it.

~~~cpp
class NumberExprAST : public ExprAST {
    double Val;
public:
    NumberExprAST(double value) {
        Val = value;
    }
};
~~~

Val belongs to the object. value is the constructor parameter.
The constructor runs when we create the object.

Members of a class are private unless a public label says otherwise.
Private data can be used inside the class, but other code cannot read it
directly. Public functions provide controlled access.

### Inheritance and virtual functions

The text ": public ExprAST" says that NumberExprAST is a kind of ExprAST.

~~~cpp
virtual llvm::Value *codegen() = 0;
~~~

Virtual means the called implementation depends on the actual object.
The "= 0" makes this a pure virtual function: derived classes must implement it
to create concrete expression objects. We cannot instantiate ExprAST itself.

Override tells C++ that a derived method implements a base virtual method.
It helps catch mistakes in the method's name or type.

~~~cpp
virtual ~ExprAST() = default;
~~~

The destructor runs when an object is destroyed. Virtual allows a derived
object to be destroyed correctly through a base pointer.
Default asks C++ to generate the ordinary destructor.
A destructor handles cleanup; the surrounding deletion mechanism handles
releasing dynamically allocated storage.

### Strings and collections

| C++ type | Simple meaning | Used for |
| --- | --- | --- |
| std::string | Text | Names and number text |
| std::vector<T> | A list that can grow | Arguments and parameter names |
| std::map<K, V> | Keys connected to values | Precedence and named LLVM values |
| std::set<T> | A collection of unique values | Detecting duplicate parameter names |

Push_back adds an item at the end of a vector.
Find looks for an entry. End means no matching entry was found.
Clear removes all entries.

Seen.insert(name).second tells us whether a name was newly inserted into a
set. False means that name was already present.

### References, pointers, and const

A reference gives another way to access an existing object.

~~~cpp
const std::string &name
~~~

This lets a function read an existing string without first making a copy.
Const prevents changing that string through this reference.

A pointer stores an address:

~~~cpp
std::istream *Input = nullptr;
~~~

Input can point to an input stream. Nullptr means it points to nothing.
The stream must remain alive while the lexer uses it.

The operators depend on context:

~~~cpp
Input = &input;       // & gets input's address.
InitializeLexer(*input); // * accesses the object pointed to by input.
Input->get();         // -> calls a method through a pointer.
~~~

In a type declaration, & declares a reference and * declares a pointer.
In an expression, they can mean address-taking and dereferencing.

### unique_ptr and ownership

~~~cpp
std::unique_ptr<ExprAST> LHS;
~~~

This pointer owns an AST object. When it is destroyed, it destroys the object
it owns. An empty unique_ptr owns nothing.

~~~cpp
auto Result = std::make_unique<NumberExprAST>(NumVal);
~~~

Make_unique creates an object and returns an owning pointer.
Auto asks C++ to work out the variable's type. The type is still fixed at
compile time.

A unique_ptr cannot be copied. Copying it would create two owners of one
object, which breaks its ownership rule.

~~~cpp
LHS = std::move(lhs);
~~~

Std::move allows ownership to be transferred by the assignment.
It does not itself carry an object from one memory address to another.
The move operation receiving its result performs the transfer.
After moving a unique_ptr, the old pointer is empty.

For a moved-from vector or string, do not assume its exact contents.
It remains valid, but its contents may have changed.

~~~cpp
return Result;
~~~

Returning a local unique_ptr by value transfers ownership automatically here.
We do not need to write std::move for that return.

### getName()

~~~cpp
const std::string &getName() const {
    return Name;
}
~~~

The first const and & say: return read-only access to the existing Name string,
without copying it.
The final const says: this method does not modify the object's ordinary members.
The returned reference remains usable only while the owning object remains alive.

### Static, extern, and the unnamed namespace

A file-level static function or variable is private to that .cpp file.

A static local variable, like the original tutorial's LastChar, keeps its value
between calls. The function still starts executing from the top each time.

Our current LastChar is a variable in an unnamed namespace. It persists between
calls and is private to lexer.cpp. The unnamed namespace keeps its names local
to the translation unit.

~~~cpp
extern std::string IdentifierStr;
~~~

Extern in this C++ header declares a variable defined elsewhere.
The actual definition appears once in lexer.cpp.
This C++ extern is separate from our toy language's extern keyword.

### Other syntax you will see

| Syntax | Meaning here |
| --- | --- |
| std:: or llvm:: | Names a member of that namespace |
| NumberExprAST::codegen | Defines that class's method outside the class |
| static_cast<char>(x) | Explicitly converts x to char |
| condition ? a : b | Use a if true; otherwise b |
| for (auto &Arg : Args) | Visit each item by reference |
| while (true) | Repeat until break, return, or an exception ends the loop |
| break | Leave the nearest loop or switch |
| return | Leave the current function |
| continue | Start the next loop iteration |
| !pointer | Test whether the pointer is empty |
| ++i | Increase i before using the result |
| i++ | Use the old value, then increase i |
| && | Both conditions must be true; skips the second if the first is false |
| \|\| | At least one condition must be true; skips the second if the first is true |

### Exceptions

~~~cpp
throw std::runtime_error("Unknown variable");
~~~

Throw stops the current path and searches for a matching catch block.
Temporary objects are cleaned up while control moves out of their scopes.

Main catches std::exception, prints its message, and returns exit code 1.
Normal successful compilation returns 0.

## 5. Tokens: token.h

A token is one recognized piece of source text.

~~~text
23 + value
~~~

becomes:

~~~text
tok_number       NumVal = 23
'+'              character token
tok_identifier   IdentifierStr = "value"
tok_eof          end of input
~~~

| Token | Code | Meaning |
| --- | --- | --- |
| tok_eof | -1 | Input has ended |
| tok_def | -2 | The keyword def |
| tok_extern | -3 | The keyword extern |
| tok_identifier | -4 | A name |
| tok_number | -5 | A number |

Special tokens have negative values. Punctuation uses its character code.
For ordinary ASCII input, '+' has code 43 and '(' has code 40.

There is no Token struct in the current implementation.
The token code and its details are stored separately:

- Return value: token kind.
- IdentifierStr: latest identifier text.
- NumVal: latest numeric value.

Only use IdentifierStr when the current token is an identifier, and NumVal
when it is a number. These globals are overwritten as later tokens are read.
A previous value may still be present when the next token is another kind.

Tok_eof and the input stream's EOF are different concepts.
The lexer notices stream EOF and translates it into the token tok_eof.

## 6. Reading characters: lexer.h and lexer.cpp

### Shared state

IdentifierStr and NumVal are defined once in lexer.cpp and declared with
extern in lexer.h.

Input points to the stream we read.
LastChar holds one character already read but not yet handled.

Keeping one character ahead lets us stop when a token ends without losing
the first character after it.

### InitializeLexer(input)

This function connects the input stream and resets lexer state:

1. Save the address of the stream in Input.
2. Set LastChar to a space.
3. Clear the previous identifier.
4. Set NumVal to 0.

The initial space causes gettok's whitespace loop to read the first real
character. Main must initialize the lexer before calling gettok.
The lexer is not designed for simultaneous independent inputs.

### Character helpers

IsSpace recognizes whitespace.
IsAlpha recognizes letters.
IsDigit recognizes digits.
IsAlnum recognizes letters or digits.

These wrappers check EOF before calling the C++ character functions and convert
ordinary characters to unsigned char. That avoids invalid negative character
arguments to those functions.

LastChar is an int because stream get() must represent both character values
and the special end-of-input value.

### gettok(): whitespace and comments

The first loop skips spaces, tabs, and newlines.
If the next character is #, another loop reads until a newline, carriage return,
or EOF. Then the outer loop checks for more whitespace and comments.

This is iterative instead of recursively calling gettok for every comment.
A long sequence of comment lines therefore does not build a deep call stack.

No comment token is returned. Comments disappear from the token stream.

### gettok(): names and keywords

If LastChar is a letter:

1. Clear IdentifierStr.
2. Add the current letter.
3. Read the next character.
4. Continue while it is a letter or digit.
5. Return tok_def for "def" or tok_extern for "extern".
6. Otherwise return tok_identifier.

For hello42 +, IdentifierStr becomes "hello42".
LastChar stops at the space. The next call skips that space and returns '+'.

An underscore currently ends a name because IsAlnum does not accept it.

### gettok(): numbers

If LastChar is a digit or dot, collect digits and dots into NumStr.

~~~text
Input: 23.5+
Collected text: "23.5"
LastChar after collecting: '+'
~~~

Strtod converts the collected C-style string into a double.

~~~cpp
char *end = nullptr;
NumVal = std::strtod(NumStr.c_str(), &end);
~~~

C_str gives access to characters ending with a zero byte.
The end pointer is updated to point just after the characters that strtod
successfully converted.

The checks mean:

- end == NumStr.c_str(): conversion did not consume a number.
- *end != '\0': some collected text was not part of the converted number.

That rejects "." and "1.2.3". It does not implement every possible numeric
validation rule; for example, floating-point overflow is not explicitly checked.

### gettok(): EOF and punctuation

At stream EOF, return tok_eof.

Otherwise:

~~~cpp
int ThisChar = LastChar;
LastChar = Input->get();
return ThisChar;
~~~

Save the current character, read the next one, and return the saved character.
This handles +, -, *, /, (, ), commas, semicolons, and other single characters.

An unknown character can pass through the lexer. The parser will normally
reject it when it does not fit the grammar.

## 7. The program tree: ast.h

AST means abstract syntax tree. It stores the structure of a program.

For 2 + 3 * 4:

~~~text
BinaryExprAST '+'
|-- NumberExprAST 2
'-- BinaryExprAST '*'
    |-- NumberExprAST 3
    '-- NumberExprAST 4
~~~

The AST does not keep every source character. Spaces and parentheses do not
need their own nodes here. Parentheses already changed how the tree was built.

| Class | Stores | Example |
| --- | --- | --- |
| ExprAST | Common interface | Any expression node |
| NumberExprAST | Val | 2.5 |
| VariableExprAST | Name | x |
| BinaryExprAST | Op, LHS, RHS | x + 2 |
| CallExprAST | Callee, Args | add(2, 3) |
| PrototypeAST | Name, parameter-name strings | add(a b) |
| FunctionAST | Proto and Body | def add(a b) a+b |

LHS means left-hand side. RHS means right-hand side.
Each side can be a whole expression.

CallExprAST's Args contains expression nodes.
PrototypeAST's Args contains strings.
Those two lists have different jobs: supplied argument expressions versus
declared parameter names.

PrototypeAST and FunctionAST do not inherit from ExprAST.
A function's interface or definition is not an ordinary expression node here.

Each expression class overrides codegen(). Its implementation is in
codegen.cpp. The AST classes describe structure, while those methods explain
how that structure becomes LLVM IR.

The forward declarations of llvm::Value and llvm::Function tell C++ that
these classes exist. Pointer declarations do not require their full definitions,
so ast.h can avoid including large LLVM headers.

## 8. Reading tokens: parser.h and parser.cpp

The parser reads tokens. It does not directly read source characters.

CurTok is the token currently being examined.
getNextToken calls gettok, stores its result in CurTok, and returns it.

A useful rule: a parser function normally consumes its own expression and
leaves CurTok at the first token after it.

Consume means advance past a token. It does not mean delete the source file.

### InitializeParser()

Set up operator precedence and reset the anonymous expression counter.

### ParseNumberExpr()

Create a NumberExprAST using NumVal, advance past the number, and return it.
The number must be saved before advancing because NumVal can change later.

### ParseParenExpr()

1. Skip '('.
2. Call ParseExpression to parse the contents.
3. Require ')'.
4. Skip ')'.
5. Return the inside expression.

The parentheses affect grouping. There is no separate parentheses AST node.

### ParseIdentifierExpr()

Save IdentifierStr before advancing. Then inspect the next token.

If it is not '(', return a VariableExprAST.
If it is '(', parse a function call:

1. Skip '('.
2. If the next token is ')', the argument list is empty.
3. Otherwise, parse an argument with ParseExpression.
4. Add its node to Args.
5. Stop at ')', or require and skip a comma before another argument.
6. Skip the closing ')'.
7. Return a CallExprAST.

For foo(2 + 3, x), the two arguments are the expression 2 + 3 and the
expression x. An argument is not limited to one token.

In Args.push_back(ParseExpression()), the returned unique_ptr is temporary.
Its ownership can be moved into the vector without an explicit std::move.

### ParsePrimary()

Choose a parser using CurTok:

| Current token | Function called |
| --- | --- |
| tok_identifier | ParseIdentifierExpr |
| tok_number | ParseNumberExpr |
| '(' | ParseParenExpr |
| Anything else | Report an error |

A primary is a basic expression piece that can be used as one side of an
operator.

### GetTokPrecedence()

Return the precedence of CurTok if it is a supported operator.
Return -1 otherwise.

The range check rejects special negative tokens and values beyond ASCII.
Map.find looks up an operator without adding a missing entry.
This differs slightly from the tutorial's map indexing approach.

### ParseExpression()

~~~cpp
auto LHS = ParsePrimary();
return ParseBinOpRHS(0, std::move(LHS));
~~~

Read the first piece. Then read any operators that follow.
The minimum allowed precedence is initially 0, so all supported operators
are accepted.

### Forward declaration

~~~cpp
static std::unique_ptr<ExprAST> ParseExpression();
~~~

This tells C++ the function's name and type before its definition.
Parentheses and call arguments need to call it earlier in the file.

This is necessary because expressions can contain parentheses and calls,
and those can contain more expressions.

## 9. Operator precedence, slowly

ParseBinOpRHS receives:

- ExprPrec: the minimum precedence this call may handle.
- LHS: the expression built so far.

It returns the larger expression formed by attaching suitable operators.

### The first check: should this call stop?

~~~cpp
int TokPrec = GetTokPrecedence();
if (TokPrec < ExprPrec)
    return LHS;
~~~

Stop if the current operator is too weak for this call.
A nonoperator has precedence -1, so it also causes a stop.

This check returns LHS unchanged. It does not put RHS into LHS.

### Reading one operator

Save the operator, skip it, and parse the next primary expression as RHS.

~~~text
2 + 3 * 4
LHS = 2
operator = +
RHS = 3
CurTok = *
~~~

### The second check: should RHS grow first?

~~~cpp
if (TokPrec < NextPrec)
    RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
~~~

If the following operator binds more tightly, finish that stronger operation
on the right before joining the outer operation.

For 2 + 3 * 4, the outer operator is + with precedence 20.
The next operator is * with precedence 40.

The recursive call receives:

~~~text
minimum precedence = 21
starting expression = 3
current operator = *
~~~

Inside that call, the outer RHS is now named LHS. It is the starting left
side of the inner expression 3 * 4.

The +1 sets the minimum just above the outer operator's precedence.
This inner call can handle stronger operators, but equal or weaker operators
will return control to the outer call.

### Combining both sides

~~~cpp
LHS = std::make_unique<BinaryExprAST>(
    static_cast<char>(BinOp), std::move(LHS), std::move(RHS));
~~~

Create a node that owns the old left expression and right expression.
Store the new combined node in LHS.
The loop can now attach more operations to that larger expression.

### Complete trace: 2 + 3 * 4

| Step | Call | LHS | Operator/current token | Action |
| --- | --- | --- | --- | --- |
| 1 | Outer, minimum 0 | 2 | +, precedence 20 | Accept + |
| 2 | Outer | 2 | * after reading RHS 3 | 40 is stronger than 20 |
| 3 | Inner, minimum 21 | 3 | *, precedence 40 | Accept * |
| 4 | Inner | 3 | Read RHS 4 | Build 3 * 4 |
| 5 | Inner | 3 * 4 | EOF, precedence -1 | Return 3 * 4 |
| 6 | Outer | 2 | RHS is now 3 * 4 | Build 2 + (3 * 4) |
| 7 | Outer | Whole expression | EOF | Return the final tree |

### Equal precedence: 20 - 5 - 3

After reading the first RHS, the current operator is the second minus.
Its precedence is equal, so the recursive condition is false.

The parser first builds 20 - 5.
The next loop iteration builds (20 - 5) - 3.
This is how left associativity comes from the code.

### Your expression: 8/2*(2 + 2)

Division and multiplication both have precedence 40.
The tree is:

~~~text
          *
         / \
        /   +
       /\  / \
      8  2 2  2
~~~

Read it as (8 / 2) * (2 + 2), which is 16.
There is no implied multiplication rule in this language. Write * explicitly.

## 10. Functions, declarations, and calls

### ParsePrototype()

A prototype is a function's name and list of parameter names.

For add(a b):

1. Require an identifier and save "add".
2. Require '('.
3. Read identifier tokens as parameter names.
4. Use Seen to reject repeated names.
5. Require ')'.
6. Advance and return PrototypeAST.

The while loop calls getNextToken in its condition, so it first moves past
'(', then past each parameter name.

### ParseDefinition()

Skip def, parse the prototype, parse one body expression, and create FunctionAST.

~~~text
def add(a b) a + b
    |------| |---|
    prototype body
~~~

### ParseExtern()

Skip extern and parse a prototype. No body is parsed.

An extern does not implement or load the external function.
Execution still needs a matching real function with a compatible calling
convention and double-based parameter and return types.

### ParseTopLevelExpr()

A top-level expression has no surrounding named function, such as 2 + 3.

LLVM instructions need somewhere to belong, so we wrap the expression in a
zero-argument function:

~~~text
__anon_expr0
__anon_expr1
~~~

AnonymousCount++ uses the current counter in the name and then increments it.

Currently user-written names cannot contain underscores, so they cannot collide
with these generated names. Adding underscores requires a new collision strategy.
One option is generated names containing a character that source names cannot use,
such as a dot.

Creating this wrapper does not call it. It only defines it in the output.

## 11. Making LLVM IR: codegen.h and codegen.cpp

Codegen means code generation.

The AST belongs to our C++ compiler. LLVM IR objects belong to LLVM.
Codegen walks the AST and creates LLVM values, functions, and instructions.

The compiler does not run each function's body while it generates IR.
A call instruction records a call for later execution.

### Four important objects

| Object | Purpose |
| --- | --- |
| TheContext | Holds shared LLVM data such as types and constants |
| TheModule | Holds the generated functions and declarations |
| Builder | Creates instructions at the current insertion point |
| NamedValues | Maps parameter names to their LLVM values |

TheContext, TheModule, and Builder use unique_ptr for their ownership.
NamedValues contains raw pointers to values owned by LLVM structures.
Those raw pointers are references to existing values, not separate owners.

### InitializeModule()

Destroy the previous builder and module first.
Then create the context, module, and builder, and clear NamedValues.

The builder and module depend on the context. They must not outlive it.
The declarations in the file also allow normal program shutdown to destroy
these objects in a suitable reverse order.

The module name comes from the input filename, or "stdin" for standard input.

### NumberExprAST::codegen()

Create an LLVM floating-point constant from Val.

~~~cpp
llvm::ConstantFP::get(*TheContext, llvm::APFloat(Val))
~~~

APFloat is LLVM's floating-point value representation.
ConstantFP is the LLVM constant used in IR.
No runtime variable or memory allocation instruction is needed for a numeric literal.

### VariableExprAST::codegen()

Look up Name in NamedValues.
Return its LLVM value if found; otherwise report "Unknown variable".

In def square(x) x*x, NamedValues maps "x" to that function's LLVM argument.
This version does not allocate a mutable storage slot for x.

### BinaryExprAST::codegen()

First generate the left child's value, then the right child's value.
Then ask Builder for the relevant operation.

| Source operator | Builder method | Typical IR |
| --- | --- | --- |
| + | CreateFAdd | fadd |
| - | CreateFSub | fsub |
| * | CreateFMul | fmul |
| / | CreateFDiv | fdiv |
| < | CreateFCmpULT and CreateUIToFP | fcmp and uitofp |

The F means floating point.

Comparison initially produces an LLVM i1 value: one bit, either 0 or 1.
CreateUIToFP turns that into the language's double value, 0.0 or 1.0.

CreateFCmpULT is an unordered less-than comparison, following the tutorial.
For ordinary numbers, it checks less-than. If either value is NaN
("not a number"), this unordered comparison also reports true.
That is a floating-point edge case to remember when changing comparisons.

Names such as addtmp are labels for LLVM results, not toy-language variables.

### Constant folding

The default IRBuilder can simplify operations whose inputs are constants.

~~~text
Source: 2 + 3 * 4
IR may contain: ret double 1.400000e+01
~~~

This is constant folding during IR construction.
Our compiler has not set up a separate optimization pass pipeline.

If an operation depends on a function argument, such as x + 2, LLVM cannot
replace it with one fixed number at this stage.

### CallExprAST::codegen()

1. Find the named function in the module.
2. Reject an unknown function.
3. Compare the number of supplied arguments with its parameter count.
4. Generate each argument expression.
5. Create a call instruction.

Args is a list of AST nodes. ArgsV is a list of generated LLVM values.
They are different representations of the same arguments at different stages.

### PrototypeAST::codegen()

Look for an existing function with this name.
If one exists, require the same parameter count.
All parameter types are double, so count is enough for this language's signature.

Otherwise:

1. Make a list containing one double type per parameter.
2. Create a function type: double return type, those parameter types, not variadic.
3. Create a function in the module with external linkage.

External linkage allows the function to be visible across module boundaries.
It does not make a function run automatically.

For a declaration without a body, update the LLVM argument names.
Old names are cleared first so swapping names does not cause suffixes.

~~~text
extern f(a b);
def f(b a) b - a;
~~~

The argument positions matter for calls; the source names inside the definition
must also match the LLVM names used by NamedValues.

### FunctionAST::codegen()

1. Generate or reuse the function declaration.
2. Reject an existing body: redefining a function is not allowed.
3. Create a basic block named entry.
4. Tell Builder to insert instructions into that block.
5. Clear NamedValues and add this function's parameters.
6. Generate the body expression.
7. Add a return instruction using the body's value.
8. Ask LLVM to verify the function.
9. Clear the builder insertion point and parameter map.

A basic block is a sequence of instructions with one ending control instruction.
Our current simple function bodies need one block. Conditionals will need more.

If body generation fails, the catch block removes the incomplete body, clears
temporary state, and rethrows the error. The declaration is kept.
Main then stops compilation.

LLVM verification checks IR structure and type rules. It does not prove that
the program calculates the answer the programmer intended.

### WriteModule()

First verify the whole module.
If no output path was supplied, print the IR to stdout.
Otherwise open the file, write the IR, flush buffered output, and check errors.

Raw_fd_ostream is LLVM's file output stream.
Std::error_code carries a file-opening error without needing a thrown exception
from the opening operation itself. Our code turns failures into runtime_error.

The whole input is compiled before this function runs.
A syntax or code-generation error therefore does not print a partial module.
A file-writing error can still leave a partially written output file.

## 12. Reading LLVM IR

Consider:

~~~llvm
define double @add(double %a, double %b) {
entry:
  %addtmp = fadd double %a, %b
  ret double %addtmp
}
~~~

| Text | Meaning |
| --- | --- |
| define | A function with a body |
| double | A double-precision floating-point type |
| @add | The function's global name |
| %a and %b | Function argument values |
| entry: | A basic block label |
| %addtmp | The result produced by the addition |
| fadd | Floating-point addition |
| ret | Return a value to the caller |

An LLVM local result name is assigned once in this form of IR.
A new calculation normally creates a new result rather than changing the old one.
This idea is called static single assignment, or SSA.

~~~llvm
%calltmp = call double @add(double 2.000000e+00, double 1.200000e+01)
~~~

This calls add with 2 and 12 and names the result calltmp.

~~~llvm
declare double @sqrt(double)
~~~

Declare means the function has a signature but no body in this module.

~~~llvm
define double @__anon_expr0() {
entry:
  ret double 1.600000e+01
}
~~~

This defines a wrapper that returns 16.
Scientific notation 1.600000e+01 means 1.6 times 10 to the power 1.
It does not mean the wrapper has already been called.

Lines beginning with a semicolon in IR are comments.
ModuleID and source_filename describe the module and source name.

File types:

| File | Contents |
| --- | --- |
| .toy | Our source language |
| .ll | Human-readable LLVM IR |
| .bc | LLVM bitcode, a binary representation of IR |
| .obj or .o | Machine-code object file for a linker |
| .exe | Executable program on Windows |

Our toycc writes .ll text. A filename extension alone does not change its format.

## 13. The controller: main.cpp

### argc and argv

Argc is the number of command-line arguments including the program name.
Argv holds their text.

~~~powershell
toycc.exe examples/add.toy -o build-x64/add.ll
~~~

The argument loop recognizes:

| Option/input | Action |
| --- | --- |
| --help | Print usage and exit successfully |
| --tokens | Print tokens instead of compiling |
| -o filename | Save LLVM IR to that filename |
| A filename | Read that input file |
| No filename | Read standard input |

Unknown options, missing output names, multiple input files, and combining
--tokens with -o are rejected.

### Input and output path check

Weakly_canonical resolves a usable normalized filesystem path.
Equivalent can detect that two existing paths refer to the same file.
These checks prevent the compiler from replacing its source with its output.

They do not stop an output path from replacing some other existing file.
Choose the output path deliberately.

### Choosing the input stream

Input initially points to std::cin, the standard input stream.
If a filename was provided, an ifstream opens it and Input points to that file.
A failed open causes an error.

The file stream remains alive until compilation ends because it is local to main.

### Token mode

InitializeLexer is called first.
PrintTokens repeatedly calls gettok and prints each token.
When tok_eof arrives, the function returns.

Token mode does not run the parser or LLVM code generator.
It can display tokens for text that is not a valid complete program.

### Compilation mode

InitializeParser sets precedence.
InitializeModule creates the LLVM objects.
getNextToken reads the first token.

Then main repeatedly checks CurTok:

- Semicolon: skip it.
- Def: parse a definition and generate its IR.
- Extern: parse a prototype and generate its declaration.
- Otherwise: parse a standalone expression and generate its wrapper.

At tok_eof, call WriteModule and return success.

Each top-level AST is used to generate IR before the next one is parsed.
The module keeps the generated IR after that temporary AST is destroyed.

In ParseDefinition()->codegen(), the arrow calls codegen through the returned
owning pointer. That temporary pointer keeps the AST alive during the call.

### Error reporting

Main catches errors using a const reference, prints error.what() to std::cerr,
and returns 1.

Std::cout is normal output. Std::cerr is error output.
This separation helps save IR without mixing error messages into it.

## 14. Building: CMakeLists.txt

CMake describes how to build the compiler. It is not itself the C++ compiler.

| CMake instruction | Purpose |
| --- | --- |
| cmake_minimum_required | Require CMake 3.20 or newer |
| cmake_policy CMP0091 | Allow selection of the MSVC runtime through CMake |
| project(... LANGUAGES C CXX) | Enable C and C++ toolchain checks |
| CMAKE_CXX_STANDARD 17 | Request C++17 |
| CMAKE_CXX_STANDARD_REQUIRED ON | Require that standard |
| CMAKE_CXX_EXTENSIONS OFF | Avoid requesting compiler-specific language extensions |
| CMAKE_MSVC_RUNTIME_LIBRARY | Match the static runtime used by this LLVM package |
| LOCAL_LLVM / LLVM_DIR | Locate the LLVM development package |
| find_package(LLVM REQUIRED CONFIG) | Read LLVM's CMake package information |
| add_executable | Name toycc and list its C++ source files |
| target_include_directories | Make LLVM headers available |
| separate_arguments | Split LLVM's supplied compile definitions into options |
| target_compile_options | Apply those options to toycc |
| llvm_map_components_to_libnames | Find libraries for LLVM core and support |
| target_link_libraries | Link those libraries into toycc |
| include(CTest) | Enable test support |
| add_test | Register the compiler_smoke test |

Headers are included by source files, so they do not need separate compilation.

Core provides LLVM IR facilities. Support provides shared utilities.
LLVM's CMake targets also bring in required dependencies.

The project prefers its local LLVM 20.1.8 package when no LLVM_DIR was supplied
and that package exists. Otherwise you must provide or install a usable package.

The original C:/LLVM tools on this machine were ARM64.
The local package is x64, matching the Visual Studio build.
Headers and libraries are needed to build against LLVM; tool executables alone
are not enough.

Configure chooses a generator and writes build files.
Build invokes the compiler and linker using those generated files.
Release selects the Release configuration in this Visual Studio build.

## 15. Tests and examples

### tests/smoke.cmake

CTest runs one registered test called compiler_smoke.
That one test executes many individual checks.

The check helper:

1. Writes a small .toy input file into the test-output directory.
2. Runs toycc.
3. Collects its exit status, stdout, and stderr.
4. Checks whether success or failure was expected.
5. Looks for an expected piece of output.

Checks include precedence, parentheses, left associativity, division, comparison,
calls, extern parameter renaming, recursion compiling successfully, empty argument
lists, comments, multiple wrappers, and invalid programs.

The recursion test checks that recursive IR can be generated. It does not execute
an infinitely recursive function.

Extra checks cover token output, writing an IR file, swapped parameter names,
and refusing to overwrite the input.

If the local LLVM tools are available, the test also:

1. Compiles add and calculate.
2. Appends a small IR main wrapper.
3. Uses llvm-as to assemble the IR into bitcode.
4. Executes it with lli.
5. Checks that calculate returns 14 by using the process exit status.

The generated main returns 0 when the answer is 14 and 1 otherwise.
This test does not need printf.

Passing these checks is useful evidence, not proof of correctness for all inputs.
Some checks match pieces of IR rather than executing every possible program.

### examples/add.toy

~~~text
def add(a b) a + b;
def calculate() add(2, 3 * 4);
calculate();
~~~

Calculate returns 14.
The last line becomes a standalone wrapper that calls calculate.

### examples/functions.toy

This example declares sqrt, defines square and distance, compares values, and
calls distance(3, 4).

It can be compiled without supplying sqrt's implementation.
Executing the distance call needs sqrt to be available in the execution setup.
An external declaration does not guarantee an external symbol will resolve.

### .gitignore and backups

Build directories, generated IR, object files, executables, downloaded tools,
and the backup directory are ignored by Git.
They are local files, not part of a normal clone.

The backup-before-completion directory preserves earlier learning versions.
Those are references, not sources used by the current build.

## 16. How to build, compile, and execute

Use PowerShell from the project folder:

~~~powershell
cd C:\compiler
~~~

### Configure and build the C++ compiler

~~~powershell
cmake -S . -B build-x64 -A x64
cmake --build build-x64 --config Release
~~~

Run configure for the initial setup or when changing build configuration.
Rebuild after changing C++ source files.

### Inspect tokens

~~~powershell
"8/2*(2 + 2)" | .\build-x64\Release\toycc.exe --tokens
~~~

Expected:

~~~text
number 8
'/'
number 2
'*'
'('
number 2
'+'
number 2
')'
eof
~~~

The pipe sends the quoted source text to standard input.
The surrounding PowerShell quotes are not part of the toy program.

### Compile to IR

~~~powershell
"8/2*(2 + 2)" | .\build-x64\Release\toycc.exe
~~~

Expected function body: ret double 1.600000e+01.

Save IR directly using -o:

~~~powershell
"8/2*(2 + 2)" | .\build-x64\Release\toycc.exe -o build-x64\expression.ll
~~~

Using -o avoids shell text-encoding surprises when saving generated IR.

### Compile a file

~~~powershell
.\build-x64\Release\toycc.exe examples\add.toy -o build-x64\add.ll
Get-Content build-x64\add.ll
~~~

Changing a .toy program does not require rebuilding toycc.
Just compile the changed input again.

### Run the tests

~~~powershell
ctest --test-dir build-x64 -C Release --output-on-failure
~~~

For detailed successful output too:

~~~powershell
ctest --test-dir build-x64 -C Release -V
~~~

### Execute the expression and print 16

The generated expression function returns double.
A conventional executable entry point needs a suitable main function.
We add one in IR, outside the toy language, because the toy language currently
cannot define the required integer-returning entry point or string constant.

First compile and make a separate runnable copy:

~~~powershell
"8/2*(2 + 2)" | .\build-x64\Release\toycc.exe -o build-x64\expression.ll
Copy-Item build-x64\expression.ll build-x64\run-expression.ll
~~~

Append this wrapper using a single-quoted PowerShell here-string:

~~~powershell
@'

@fmt = private constant [4 x i8] c"%g\0A\00"
declare i32 @printf(ptr, ...)

define i32 @main() {
entry:
  %answer = call double @__anon_expr0()
  call i32 (ptr, ...) @printf(ptr @fmt, double %answer)
  ret i32 0
}
'@ | Add-Content build-x64\run-expression.ll

& ".\tools\clang+llvm-20.1.8-x86_64-pc-windows-msvc\bin\lli.exe" build-x64\run-expression.ll
~~~

Expected output: 16.

The wrapper's extra IR means:

- @fmt stores four bytes: percent, g, newline, and a zero terminator.
- i8 is an 8-bit integer used here for each character byte.
- Backslash 0A represents a newline byte; backslash 00 is the terminator.
- printf is an external variadic function: it can receive extra arguments.
- %g asks printf to display the supplied double.
- i32 is a 32-bit integer, used for main's return code.
- ret i32 0 reports successful process completion.

The local execution setup used earlier resolved printf successfully.
Other toolchains may need extra runtime symbol setup.

For the add.toy example, call @calculate() in the wrapper to print 14.
For the standalone arithmetic expression, call @__anon_expr0() to print 16.
The function called by the wrapper determines which program result is printed.

Start from a fresh copy each time before appending the wrapper.
Appending it twice creates duplicate definitions.

## 17. Two full worked examples

### Example A: 2 + 3 * 4

Main initializes the lexer, parser, and module.

The lexer returns:

~~~text
number 2, '+', number 3, '*', number 4, eof
~~~

ParseTopLevelExpr calls ParseExpression.
ParsePrimary reads 2.
ParseBinOpRHS reads + and notices the stronger *.
The recursive call groups 3 * 4 first.
The AST represents 2 + (3 * 4).

The top-level parser adds a prototype named __anon_expr0 with no parameters.

Function codegen creates that LLVM function and its entry block.
Number codegen makes constants.
Binary codegen asks Builder for multiplication and addition.
Because both operations have constant inputs, they can fold into 14.
Function codegen emits a return instruction.

WriteModule prints approximately:

~~~llvm
define double @__anon_expr0() {
entry:
  ret double 1.400000e+01
}
~~~

No printing call or conventional main exists yet.
The compiler has generated a program representation, not executed it.

### Example B: def add(a b) a+b; add(2, 3);

1. Main sees tok_def.
2. ParseDefinition skips def.
3. ParsePrototype saves add and the names a and b.
4. ParseExpression builds a BinaryExprAST for a + b.
5. FunctionAST owns the prototype and body.
6. Codegen creates a function taking two doubles and returning double.
7. NamedValues maps a and b to its LLVM arguments.
8. Variable codegen finds those values.
9. Binary codegen creates fadd; function codegen creates ret.
10. Main skips the semicolon.
11. The next item starts with add, so it is parsed as a top-level expression.
12. ParseIdentifierExpr sees '(' and reads argument nodes for 2 and 3.
13. ParseTopLevelExpr wraps the call in __anon_expr0.
14. Call codegen finds add, checks two arguments, and emits a call.
15. The wrapper returns that call's result.
16. The module is verified and written at EOF.

The generated call records work to happen at runtime.
Our baseline does not inline add or execute its body during compilation.

## 18. Errors, limits, and differences from the tutorial

### Which stage reports an error?

| Input or situation | Stage | Why |
| --- | --- | --- |
| 1.2.3 | Lexer | Malformed number |
| 2 + ; | Parser | An expression is missing after + |
| (2 + 3 | Parser | Closing parenthesis is missing |
| def f(x x) x | Parser | Repeated parameter name |
| def f(x) y | Codegen | y is not a known parameter |
| missing(1) | Codegen | No declaration for missing |
| Calling a one-parameter function with two arguments | Codegen | Argument count mismatch |
| Defining the same function twice | Codegen | Existing function body |
| Input file cannot be opened | Main | File I/O failure |
| IR output cannot be written | WriteModule | Output I/O failure |

Syntax errors concern structure. Unknown names and incompatible calls concern
meaning; our implementation checks much of that meaning during codegen.

### Current limits

- Only double values; no strings, arrays, objects, or separate booleans.
- Only parameters as variables; no local declarations or assignments.
- No unary minus, conditionals, loops, custom operators, or built-in run mode.
- No source line and column in error messages.
- No independent optimizer pipeline or object-file emitter.
- No designed support for underscore names yet.
- Numeric exponents and overflow checks are not implemented.
- Unknown single characters are not rejected directly by the lexer.
- Division by zero has no language-level error check.
- Compilation stops at the first error.
- Shared global state means the frontend is not designed for concurrent use.

### Main changes from the tutorial

The same main stages and many names remain, but this version has separate files,
stream input, a batch CLI, division, extra validation, unique wrapper names,
CMake, examples, and regression checks.

Tutorial error helpers often return null pointers. This version throws exceptions.
Do not mix assumptions from the earlier handwritten learning code with the
completed implementation without checking the actual files.

### Common misunderstandings

| Thought | What actually happens |
| --- | --- |
| --tokens calculates the answer | It only displays tokens |
| Reading a .ll file runs it | It only displays generated instructions |
| A return value automatically appears on screen | Printing requires a printing call |
| NumVal holds every number | It only holds the most recently scanned number |
| Static restarts execution where it stopped | The function starts at the top; stored state persists |
| Std::move copies the expression tree | Ownership is transferred by the move operation |
| LHS is always one number | It can be a whole subtree |
| Returning LHS on low precedence replaces it with RHS | It returns the expression built so far |
| __anon_expr0 is main | It is a double-returning wrapper, not the conventional entry point |
| Extern supplies the external implementation | It only declares a signature |
| Ret double 1.600000e+01 means 1.6 | It means return 16 |
| All compiler work is performed by LLVM | Our lexer and parser are C++; LLVM is used for IR generation onward |

## 19. Where to add features

Start by writing a tiny input and the behavior you expect.
Change one feature at a time, rebuild, and add a regression check.

| Feature | Main places to change | First test |
| --- | --- | --- |
| Underscores in names | Lexer and internal wrapper naming | my_func and _value tokenize as names |
| Unary minus | AST/parser/codegen, or a simple 0-x representation | -5 + 2 gives -3 |
| Greater-than | Precedence table and binary codegen | 3 > 2 gives 1 |
| Equality == | Lexer, parser precedence, codegen | 2 == 2 gives 1 |
| Source positions in errors | Lexer token information and diagnostics | Missing ')' reports a location |
| If/then/else | Tokens, lexer, AST, parser, codegen | Choose one of two values |
| Local variables | Grammar, AST, scope handling, codegen | Bind a name and use it |
| Built-in execution | CLI, LLVM execution integration, runtime setup | --run prints an expression's value |

A two-character operator such as == needs explicit token recognition.
The current punctuation path returns one character at a time.

For underscore support, allow underscore both at the start and later in a name.
Also move internal wrapper names out of the user-name space, or detect/reserve
collisions. Changing only the lexer can allow a user to collide with __anon_expr0.

For unary minus, define its precedence deliberately.
Test -5, -x, -(2+3), and 2 * -3 rather than just one numeric literal.

For if/then/else, LLVM will need multiple basic blocks and a way to combine the
chosen values. That is a larger step than adding another arithmetic operator.

## 20. Practice questions and glossary

### Suggested reading sessions

| Session | Read | Goal |
| --- | --- | --- |
| 1 | token.h and lexer.cpp | Explain how 23 + x becomes tokens |
| 2 | ast.h | Draw a tree for 2 + 3 * 4 |
| 3 | Parser primary functions | Explain numbers, parentheses, names, and calls |
| 4 | ParseBinOpRHS | Trace stronger and equal precedence |
| 5 | Prototype and function parsing | Explain definitions versus calls |
| 6 | codegen.cpp | Read the IR for add(a b) |
| 7 | main.cpp, CMake, tests | Build, compile, test, and execute |

### Questions to answer without looking

1. Why is LastChar an int rather than a char?
2. What does the lexer save when it returns tok_number?
3. Why save IdName before calling getNextToken?
4. How does the parser distinguish x from foo(2)?
5. What is different between prototype Args and call Args?
6. Why can a unique_ptr not be copied?
7. Why does an inner ParseBinOpRHS call receive the outer RHS as its LHS?
8. What happens when two operators have equal precedence?
9. Why is 8/2*(2+2) equal to 16 here?
10. Where does a parameter name become an LLVM value?
11. Why does extern sqrt(x) not guarantee that execution can find sqrt?
12. Why can the generated arithmetic IR contain one constant return?
13. Why does generating __anon_expr0 not print its value?
14. Which command rebuilds the C++ compiler after source changes?

### Answer hints

1. A stream needs to report ordinary characters and EOF.
2. The numeric value goes in NumVal.
3. The lexer can overwrite IdentifierStr.
4. Look for '(' after the identifier.
5. Parameter-name strings versus expression nodes.
6. It represents one exclusive owner.
7. That expression is the starting left side of the stronger inner operation.
8. The parser combines the left operation first.
9. * and / have equal precedence and group left to right.
10. Function codegen fills NamedValues using LLVM arguments.
11. A declaration gives no implementation.
12. IRBuilder can fold constant arithmetic.
13. A definition is not a call, and a return is not a print operation.
14. cmake --build build-x64 --config Release.

### Short glossary

| Word | Meaning |
| --- | --- |
| Source | The program text |
| Token | One recognized word, number, or symbol |
| Lexer | Converts characters to tokens |
| Grammar | Rules describing valid program structure |
| Parser | Uses grammar to build structure from tokens |
| AST | A tree representing that structure |
| Node | One object in a tree |
| Literal | A value written directly, such as 2.5 |
| Identifier | A name, such as x or add |
| Parameter | A name declared by a function |
| Argument | An expression supplied in a call |
| Prototype | A function's name and parameter information |
| Precedence | Which operator binds more tightly |
| Associativity | How equal-precedence operators group |
| Recursion | A function calling itself, directly or indirectly |
| Ownership | Responsibility for an object's lifetime |
| IR | Intermediate representation |
| Module | An LLVM container for functions and related data |
| Basic block | Instructions with one ending control instruction |
| Constant folding | Replacing constant operations with their results |
| Linker | Joins object code and libraries |
| Runtime | The period when a generated program executes |
| JIT | Compilation to machine code as part of execution |
| Exit code | A process result; 0 usually indicates success |

## 21. References and acknowledgements

The implementation is based on the LLVM Project's Kaleidoscope tutorial.
Use these notes beside the tutorial, remembering the repository-specific
differences explained above.

- [Chapter 1: language and lexer](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl01.html)
- [Chapter 2: parser and AST](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html)
- [Chapter 3: LLVM IR generation](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html)

Thanks to the LLVM tutorial authors and contributors for the reference design,
and to LLVM's contributors for the libraries and tools.
ChatGPT/Codex helped explain and implement this learning project and prepare
these notes. The source files remain the reference for what this version does.
