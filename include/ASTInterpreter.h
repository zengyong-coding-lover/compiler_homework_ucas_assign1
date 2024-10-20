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

    virtual bool VisitIntegerLiteral(IntegerLiteral *int_literal);
    virtual bool VisitBinaryOperator(BinaryOperator *bop);
    virtual bool VisitUnaryOperator(UnaryOperator *uop);
    virtual bool VisitDeclRefExpr(DeclRefExpr *expr);
    virtual bool VisitCastExpr(CastExpr *expr);
    virtual bool VisitCallExpr(CallExpr *call);
    virtual bool VisitCStyleCastExpr(CStyleCastExpr *expr);
    virtual bool VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *expr);
    virtual bool VisitParenExpr(ParenExpr *parent);
    virtual bool VisitReturnStmt(ReturnStmt *retstmt);
    virtual bool VisitDeclStmt(DeclStmt *declstmt);
    virtual bool VisitIfStmt(IfStmt *ifstmt);
    virtual bool VisitWhileStmt(WhileStmt *whilestmt);
    virtual bool VisitForStmt(ForStmt *forstmt);
    virtual bool VisitArraySubscriptExpr(ArraySubscriptExpr *array);
    virtual bool VisitStmt(Stmt *stmt);
    virtual bool VisitVarDecl(VarDecl *vardecl) {
        return true;
    }
    bool _VisitExpr_(Expr *exp);
    bool _VisitStmt_(Stmt *stmt);

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