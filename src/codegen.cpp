#include "codegen.h"
#include "ast.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <stdexcept>

// These are the Chapter 3 objects, kept private to this source file.
static std::unique_ptr<llvm::LLVMContext> TheContext;
static std::unique_ptr<llvm::Module> TheModule;
static std::unique_ptr<llvm::IRBuilder<>> Builder;
static std::map<std::string, llvm::Value *> NamedValues;

static void LogErrorV(const std::string &message) {
    throw std::runtime_error(message);
}
void InitializeModule(const std::string &name) {
    // Destroy dependents before replacing their context.
    Builder.reset();
    TheModule.reset();
    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>(name, *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
    NamedValues.clear();
}
llvm::Value *NumberExprAST::codegen() {
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(Val));
}
llvm::Value *VariableExprAST::codegen() {
    auto entry = NamedValues.find(Name);
    if (entry == NamedValues.end()) LogErrorV("Unknown variable: " + Name);
    return entry->second;
}
llvm::Value *BinaryExprAST::codegen() {
    llvm::Value *L = LHS->codegen();
    llvm::Value *R = RHS->codegen();
    switch (Op) {
    case '+': return Builder->CreateFAdd(L, R, "addtmp");
    case '-': return Builder->CreateFSub(L, R, "subtmp");
    case '*': return Builder->CreateFMul(L, R, "multmp");
    case '/': return Builder->CreateFDiv(L, R, "divtmp");
    case '<': {
        // Comparisons produce a boolean; the language represents it as 0.0 or 1.0.
        auto *comparison = Builder->CreateFCmpULT(L, R, "cmptmp");
        return Builder->CreateUIToFP(comparison, llvm::Type::getDoubleTy(*TheContext), "booltmp");
    }
    default:
        LogErrorV("Unknown binary operator");
        return nullptr;
    }
}
llvm::Value *CallExprAST::codegen() {
    llvm::Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF) LogErrorV("Unknown function: " + Callee);
    if (CalleeF->arg_size() != Args.size())
        LogErrorV("Wrong number of arguments for: " + Callee);

    std::vector<llvm::Value *> ArgsV;
    for (auto &Arg : Args)
        ArgsV.push_back(Arg->codegen());
    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}
llvm::Function *PrototypeAST::codegen() {
    llvm::Function *F = TheModule->getFunction(Name);
    if (F && F->arg_size() != Args.size())
        LogErrorV("Conflicting declaration for: " + Name);
    if (!F) {
        std::vector<llvm::Type *> Doubles(Args.size(), llvm::Type::getDoubleTy(*TheContext));
        auto *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), Doubles, false);
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, TheModule.get());
    }
    // A definition may use different parameter names than its earlier extern.
    if (F->empty()) {
        // Clear old names first, so swapped names do not gain LLVM suffixes.
        for (auto &Arg : F->args()) Arg.setName("");
        unsigned index = 0;
        for (auto &Arg : F->args())
            Arg.setName(Args[index++]);
    }
    return F;
}
llvm::Function *FunctionAST::codegen() {
    llvm::Function *F = Proto->codegen();
    if (!F->empty()) LogErrorV("Function already defined: " + Proto->getName());

    auto *BB = llvm::BasicBlock::Create(*TheContext, "entry", F);
    Builder->SetInsertPoint(BB);
    NamedValues.clear();
    for (auto &Arg : F->args())
        NamedValues[Arg.getName().str()] = &Arg;

    try {
        llvm::Value *RetVal = Body->codegen();
        Builder->CreateRet(RetVal);
        if (llvm::verifyFunction(*F, &llvm::errs()))
            LogErrorV("LLVM rejected function: " + Proto->getName());
    } catch (...) {
        // Keep the declaration, but remove an incomplete function body.
        Builder->ClearInsertionPoint();
        F->deleteBody();
        NamedValues.clear();
        throw;
    }
    Builder->ClearInsertionPoint();
    NamedValues.clear();
    return F;
}
void WriteModule(const std::string &outputPath) {
    if (llvm::verifyModule(*TheModule, &llvm::errs()))
        LogErrorV("LLVM rejected the module");
    if (outputPath.empty()) {
        TheModule->print(llvm::outs(), nullptr);
        return;
    }
    std::error_code error;
    llvm::raw_fd_ostream output(outputPath, error, llvm::sys::fs::OF_Text);
    if (error) LogErrorV("Cannot open output: " + error.message());
    TheModule->print(output, nullptr);
    output.flush();
    if (output.has_error()) LogErrorV("Failed to write output file");
}
