#ifndef __ASTInterpreter__
#define __ASTInterpreter__
#include "Environment.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

class InterpreterVisitor : public EvaluatedExprVisitor<InterpreterVisitor> {
public:
    explicit InterpreterVisitor(const ASTContext &context, Environment *env)
        : EvaluatedExprVisitor(context)
        , mEnv(env) { }
    virtual ~InterpreterVisitor() {
    }

    virtual void VisitIntegerLiteral(IntegerLiteral *int_literal);
    virtual void VisitBinaryOperator(BinaryOperator *bop);
    virtual void VisitUnaryOperator(UnaryOperator *uop);
    virtual void VisitDeclRefExpr(DeclRefExpr *expr);
    virtual void VisitCastExpr(CastExpr *expr);
    virtual void VisitCallExpr(CallExpr *call);
    virtual void VisitReturnStmt(ReturnStmt *retstmt);
    virtual void VisitDeclStmt(DeclStmt *declstmt);
    virtual void VisitIfStmt(IfStmt *ifstmt);
    virtual void VisitWhileStmt(WhileStmt *whilestmt);
    virtual void VisitForStmt(ForStmt *forstmt);
    void _VisitExpr_(Expr *exp);
    void _VisitStmt_(Stmt *stmt);

private:
    Environment *mEnv;
};

class InterpreterConsumer : public ASTConsumer {
public:
    explicit InterpreterConsumer(const ASTContext &context)
        : mEnv()
        , mVisitor(context, &mEnv) {
    }
    virtual ~InterpreterConsumer() { }

    virtual void HandleTranslationUnit(clang::ASTContext &Context);

private:
    Environment mEnv;
    InterpreterVisitor mVisitor;
};

class InterpreterClassAction : public ASTFrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &Compiler, llvm::StringRef InFile);
};

#endif