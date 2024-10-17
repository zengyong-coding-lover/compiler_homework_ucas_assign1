#ifndef __SUPPROT__
#define __SUPPORT__
#include <stdio.h>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

int cal_binary(int val1, int val2, BinaryOperatorKind kind);
int cal_unary(int val, UnaryOperatorKind kind);
int caluate_exp(Expr *exp); 
bool is_global_var(Decl *decl);
#endif