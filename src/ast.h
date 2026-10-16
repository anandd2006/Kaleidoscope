#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Forward declarations avoid pulling LLVM headers into the parser.
namespace llvm { class Value; class Function; }

// Chapter 2 builds these nodes; Chapter 3's codegen() emits LLVM IR.
class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual llvm::Value *codegen() = 0;
};

class NumberExprAST : public ExprAST {
    double Val;
public:
    NumberExprAST(double value) { Val = value; }
    llvm::Value *codegen() override;
};

class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(const std::string &name) { Name = name; }
    llvm::Value *codegen() override;
};

class BinaryExprAST : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS;
    std::unique_ptr<ExprAST> RHS;
public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs,
                  std::unique_ptr<ExprAST> rhs) {
        Op = op;
        LHS = std::move(lhs);
        RHS = std::move(rhs);
    }
    llvm::Value *codegen() override;
};

class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;
public:
    CallExprAST(std::string callee, std::vector<std::unique_ptr<ExprAST>> args) {
        Callee = callee;
        Args = std::move(args);
    }
    llvm::Value *codegen() override;
};

class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAST(std::string name, std::vector<std::string> args) {
        Name = name;
        Args = std::move(args);
    }
    const std::string &getName() const { return Name; }
    llvm::Function *codegen();
};

class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;
public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto, std::unique_ptr<ExprAST> body) {
        Proto = std::move(proto);
        Body = std::move(body);
    }
    llvm::Function *codegen();
};
