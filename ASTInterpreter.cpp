//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

#include "Environment.h"

class InterpreterVisitor : public EvaluatedExprVisitor<InterpreterVisitor> {
public:
    explicit InterpreterVisitor(const ASTContext &context, Environment *env)
        : EvaluatedExprVisitor(context)
        , mEnv(env) { }
    virtual ~InterpreterVisitor() { }

    virtual void VisitIntegerLiteral(IntegerLiteral *int_literal) {
        mEnv->intliteral(int_literal);
    }
    virtual void VisitBinaryOperator(BinaryOperator *bop) {
        VisitStmt(bop);
        mEnv->binop(bop);
    }
    virtual void VisitUnaryOperator(UnaryOperator *uop){
        VisitStmt(uop);
        mEnv->unop(uop);
    }
    virtual void VisitDeclRefExpr(DeclRefExpr *expr) {
        VisitStmt(expr);
        mEnv->declref(expr);
    }
    virtual void VisitCastExpr(CastExpr *expr) {
        VisitStmt(expr);
        mEnv->cast(expr);
    }
    virtual void VisitCallExpr(CallExpr *call) {
        VisitStmt(call);
        mEnv->call(call);

        FunctionDecl *callee = call->getDirectCallee();
        if (Stmt *body = callee->getBody()) {
            VisitStmt(body);
        }

        mEnv->finish_call(call);
    }
    virtual void VisitReturnStmt(ReturnStmt *retstmt) {
        VisitStmt(retstmt);
        mEnv->ret(retstmt);
    }
    virtual void VisitDeclStmt(DeclStmt *declstmt) {
        VisitStmt(declstmt);
        mEnv->decl(declstmt);
    }
    virtual void VisitIfStmt(IfStmt *ifstmt) {
        Expr *cond = ifstmt->getCond();
        if (BinaryOperator *bop = llvm::dyn_cast<BinaryOperator>(cond))
            VisitBinaryOperator(bop);
        else if (DeclRefExpr *declRef = llvm::dyn_cast<DeclRefExpr>(cond))
            VisitDeclRefExpr(declRef);
        else if (CallExpr *call = llvm::dyn_cast<CallExpr>(cond))
            VisitCallExpr(call);
        else if (CastExpr *cast = llvm::dyn_cast<CastExpr>(cond))
            VisitCastExpr(cast);
        else
            // If it's an expression type that we haven't handled explicitly, visit it as a generic statement
            VisitStmt(cond);

        Stmt *next = mEnv->iff(ifstmt);
        // next->dump();
        if (next) {
            if (ReturnStmt *retstmt = dyn_cast<ReturnStmt>(next))
                VisitReturnStmt(retstmt);
            else if (DeclStmt *declstmt = dyn_cast<DeclStmt>(next))
                VisitDeclStmt(declstmt);
            else if (IfStmt *ifstmt = dyn_cast<IfStmt>(next))
                VisitIfStmt(ifstmt);
            else
                VisitStmt(next);
        }
    }

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

    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        TranslationUnitDecl *decl = Context.getTranslationUnitDecl();
        mEnv.init(decl);

        FunctionDecl *entry = mEnv.getEntry();
        mVisitor.VisitStmt(entry->getBody());
    }

private:
    Environment mEnv;
    InterpreterVisitor mVisitor;
};

class InterpreterClassAction : public ASTFrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
        return std::unique_ptr<clang::ASTConsumer>(
            new InterpreterConsumer(Compiler.getASTContext()));
    }
};

int main(int argc, char **argv) {
    if (argc > 1) {
        // llvm::errs() << argv[1];
        clang::tooling::runToolOnCode(std::unique_ptr<clang::FrontendAction>(new InterpreterClassAction), argv[1]);
    }
}
