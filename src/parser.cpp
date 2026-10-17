#include "parser.h"
#include "lexer.h"
#include "token.h"
#include <map>
#include <set>
#include <stdexcept>

int CurTok = 0;
static std::map<char, int> BinopPrecedence;
static unsigned AnonymousCount = 0;

void InitializeParser() {
    BinopPrecedence = {{'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}, {'/', 40}};
    AnonymousCount = 0;
}
int getNextToken() { return CurTok = gettok(); }

// Exceptions stop compilation on the first error; main prints the message.
static void LogError(const std::string &message) {
    throw std::runtime_error(message);
}
static std::unique_ptr<ExprAST> ParseExpression();

static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken(); // Consume the number.
    return Result;
}
static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken(); // Consume '('.
    auto V = ParseExpression();
    if (CurTok != ')') LogError("Expected ')'");
    getNextToken(); // Consume ')'.
    return V;
}
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return std::make_unique<VariableExprAST>(IdName);

    getNextToken(); // Consume '('.
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')') {
        while (true) {
            Args.push_back(ParseExpression());
            if (CurTok == ')') break;
            if (CurTok != ',') LogError("Expected ')' or ',' in argument list");
            getNextToken(); // Consume ','.
        }
    }
    getNextToken(); // Consume ')'.
    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}
static std::unique_ptr<ExprAST> ParseIfExpr(){
    getNextToken();
    auto Cond= ParseExpression();
    if(CurTok!=tok_then){
        LogError("Expected 'then'");

    }
    getNextToken();

    auto Then = ParseExpression();

    if(CurTok!=tok_else){
        LogError("Expected 'else'");
    }
    getNextToken();

    auto Else=ParseExpression();
    return std::make_unique<IfExprAST>(std::move(Cond),std::move(Then),std::move(Else));
}
static std::unique_ptr<ExprAST> ParsePrimary();
static std::unique_ptr<ExprAST>ParseUnaryMinusExpr(){
    getNextToken();
    auto Operand = ParsePrimary();
    auto Zero = std::make_unique<NumberExprAST>(0.0);
    return std::make_unique<BinaryExprAST>('-',std::move(Zero),std::move(Operand));

}

static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
    case tok_identifier: return ParseIdentifierExpr();
    case tok_number: return ParseNumberExpr();
    case '(': return ParseParenExpr();
    case '-': return ParseUnaryMinusExpr();
    case tok_if:return ParseIfExpr();
    default:
        LogError("Unknown token when expecting an expression");
        return nullptr;
    }
}
static int GetTokPrecedence() {
    if (CurTok < 0 || CurTok > 127) return -1;
    auto entry = BinopPrecedence.find(static_cast<char>(CurTok));
    if (entry == BinopPrecedence.end()) return -1;
    return entry->second;
}
static std::unique_ptr<ExprAST>
ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
    while (true) {
        int TokPrec = GetTokPrecedence();
        // A weak operator belongs to the caller; a nonoperator ends the expression.
        if (TokPrec < ExprPrec) return LHS;

        int BinOp = CurTok;
        getNextToken();
        auto RHS = ParsePrimary();

        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            // Finish stronger operations on the right before combining.
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
        }
        LHS = std::make_unique<BinaryExprAST>(
            static_cast<char>(BinOp), std::move(LHS), std::move(RHS));
    }
}
static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    return ParseBinOpRHS(0, std::move(LHS));
}
static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != tok_identifier) LogError("Expected function name in prototype");
    std::string FnName = IdentifierStr;
    getNextToken();
    if (CurTok != '(') LogError("Expected '(' in prototype");

    std::vector<std::string> ArgNames;
    std::set<std::string> Seen;
    // Kaleidoscope prototypes separate names with spaces, not commas.
    while (getNextToken() == tok_identifier) {
        if (!Seen.insert(IdentifierStr).second)
            LogError("Duplicate parameter: " + IdentifierStr);
        ArgNames.push_back(IdentifierStr);
    }
    if (CurTok != ')') LogError("Expected ')' in prototype (parameters use spaces)");
    getNextToken();
    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}
std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // Consume 'def'.
    auto Proto = ParsePrototype();
    auto Body = ParseExpression();
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
}
std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken(); // Consume 'extern'.
    return ParsePrototype();
}
std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    auto Body = ParseExpression();
    // Each standalone expression becomes a separate zero-argument function.
    // Underscores are not legal source identifiers, avoiding name collisions.
    auto Proto = std::make_unique<PrototypeAST>(
        "__anon_expr" + std::to_string(AnonymousCount++), std::vector<std::string>{});
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
}
