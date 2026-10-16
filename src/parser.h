#pragma once
#include "ast.h"

extern int CurTok;
int getNextToken();
void InitializeParser();
std::unique_ptr<PrototypeAST> ParseExtern();
std::unique_ptr<FunctionAST> ParseDefinition();
std::unique_ptr<FunctionAST> ParseTopLevelExpr();
