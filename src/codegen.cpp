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

// Use the tutorial's LLVM names in this implementation file.
// Chapters 3 and 5:
// https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html
// https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl05.html
using namespace llvm;

static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
static std::map<std::string, Value *> NamedValues;

// Project difference: main handles exceptions, so errors still throw.
static Value *LogErrorV(const std::string &Str) {
    throw std::runtime_error(Str);
}

void InitializeModule(const std::string &name) {
    Builder.reset();
    TheModule.reset();

    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>(name, *TheContext);
    Builder = std::make_unique<IRBuilder<>>(*TheContext);
    NamedValues.clear();
}

Value *NumberExprAST::codegen() {
    return ConstantFP::get(*TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen() {
    // Look up this variable in the current function.
    Value *V = NamedValues[Name];
    if (!V)
        return LogErrorV("Unknown variable: " + Name);
    return V;
}

Value *BinaryExprAST::codegen() {
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if (!L || !R)
        return nullptr;

    switch (Op) {
    case '+':
        return Builder->CreateFAdd(L, R, "addtmp");
    case '-':
        return Builder->CreateFSub(L, R, "subtmp");
    case '*':
        return Builder->CreateFMul(L, R, "multmp");
    case '/': // Project addition.
        return Builder->CreateFDiv(L, R, "divtmp");
    case '<':
        L = Builder->CreateFCmpULT(L, R, "cmptmp");
        // Turn the one-bit result into our language's double.
        return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
    default:
        return LogErrorV("Unknown binary operator");
    }
}

Value *CallExprAST::codegen() {
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        return LogErrorV("Unknown function: " + Callee);

    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Wrong number of arguments for: " + Callee);

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }

    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *IfExprAST::codegen() {
    Value *CondV = Cond->codegen();
    if (!CondV)
        return nullptr;

    // Compare the condition with zero to get a true/false value.
    CondV = Builder->CreateFCmpONE(
        CondV, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");

    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    // Attach 'then' now; attach 'else' and 'ifcont' later, as in Chapter 5.
    BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "then", TheFunction);
    BasicBlock *ElseBB = BasicBlock::Create(*TheContext, "else");
    BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "ifcont");

    // The try/catch only adapts the tutorial to our exception-based errors.
    try {
        Builder->CreateCondBr(CondV, ThenBB, ElseBB);

        // Generate the then expression.
        Builder->SetInsertPoint(ThenBB);
        Value *ThenV = Then->codegen();
        if (!ThenV)
            return nullptr;

        Builder->CreateBr(MergeBB);
        // A nested if may have changed the current block.
        ThenBB = Builder->GetInsertBlock();

        // Attach the else block and generate its expression.
        TheFunction->insert(TheFunction->end(), ElseBB);
        Builder->SetInsertPoint(ElseBB);
        Value *ElseV = Else->codegen();
        if (!ElseV)
            return nullptr;

        Builder->CreateBr(MergeBB);
        ElseBB = Builder->GetInsertBlock();

        // Attach the meeting point.
        TheFunction->insert(TheFunction->end(), MergeBB);
        Builder->SetInsertPoint(MergeBB);
        PHINode *PN = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, "iftmp");

        // Choose the value from the branch that actually ran.
        PN->addIncoming(ThenV, ThenBB);
        PN->addIncoming(ElseV, ElseBB);
        return PN;
    } catch (...) {
        // Let FunctionAST's error cleanup own and remove unfinished blocks too.
        if (!ElseBB->getParent())
            TheFunction->insert(TheFunction->end(), ElseBB);
        if (!MergeBB->getParent())
            TheFunction->insert(TheFunction->end(), MergeBB);
        throw;
    }
}

Function *PrototypeAST::codegen() {
    // Project checks: reuse declarations and reject conflicting parameter counts.
    Function *F = TheModule->getFunction(Name);
    if (F && F->arg_size() != Args.size()) {
        LogErrorV("Conflicting declaration for: " + Name);
        return nullptr;
    }

    if (!F) {
        std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
        FunctionType *FT =
            FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);
        F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
    }

    // Project fix: definitions may rename an earlier extern's parameters.
    if (F->empty()) {
        for (auto &Arg : F->args())
            Arg.setName("");

        unsigned Idx = 0;
        for (auto &Arg : F->args())
            Arg.setName(Args[Idx++]);
    }

    return F;
}

Function *FunctionAST::codegen() {
    // PrototypeAST also handles an earlier extern and parameter-name updates.
    Function *TheFunction = Proto->codegen();
    if (!TheFunction)
        return nullptr;

    if (!TheFunction->empty()) {
        LogErrorV("Function already defined: " + Proto->getName());
        return nullptr;
    }

    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    NamedValues.clear();
    for (auto &Arg : TheFunction->args())
        NamedValues[Arg.getName().str()] = &Arg;

    try {
        if (Value *RetVal = Body->codegen()) {
            Builder->CreateRet(RetVal);

            if (verifyFunction(*TheFunction, &errs()))
                LogErrorV("LLVM rejected function: " + Proto->getName());

            Builder->ClearInsertionPoint();
            NamedValues.clear();
            return TheFunction;
        }

        LogErrorV("Could not generate function: " + Proto->getName());
    } catch (...) {
        // Keep the extern declaration while removing a failed definition's body.
        Builder->ClearInsertionPoint();
        TheFunction->deleteBody();
        NamedValues.clear();
        throw;
    }
    return nullptr;
}

// Project output support: emit one verified module to stdout or a file.
void WriteModule(const std::string &outputPath) {
    if (verifyModule(*TheModule, &errs()))
        LogErrorV("LLVM rejected the module");

    if (outputPath.empty()) {
        TheModule->print(outs(), nullptr);
        return;
    }

    std::error_code Error;
    raw_fd_ostream Output(outputPath, Error, sys::fs::OF_Text);
    if (Error)
        LogErrorV("Cannot open output: " + Error.message());

    TheModule->print(Output, nullptr);
    Output.flush();
    if (Output.has_error())
        LogErrorV("Failed to write output file");
}
