#include "ASTInterpreter.h"

void InterpreterVisitor::VisitIntegerLiteral(IntegerLiteral *int_literal) {
    mEnv->intliteral(int_literal);
}

void InterpreterVisitor::VisitBinaryOperator(BinaryOperator *bop) {
    VisitStmt(bop);
    mEnv->binop(bop);
}
void InterpreterVisitor::VisitUnaryOperator(UnaryOperator *uop) {
    VisitStmt(uop);
    mEnv->unop(uop);
}
void InterpreterVisitor::VisitDeclRefExpr(DeclRefExpr *expr) {
    VisitStmt(expr);
    mEnv->declref(expr);
}
void InterpreterVisitor::VisitCastExpr(CastExpr *expr) {
    VisitStmt(expr);
    mEnv->cast(expr);
}
void InterpreterVisitor::VisitCallExpr(CallExpr *call) {
    VisitStmt(call);
    mEnv->call(call);

    FunctionDecl *callee = call->getDirectCallee();
    if (Stmt *body = callee->getBody()) {
        VisitStmt(body);
    }

    mEnv->finish_call(call);
}
void InterpreterVisitor::VisitReturnStmt(ReturnStmt *retstmt) {
    VisitStmt(retstmt);
    mEnv->ret(retstmt);
}
void InterpreterVisitor::VisitDeclStmt(DeclStmt *declstmt) {
    VisitStmt(declstmt);
    mEnv->decl(declstmt);
}
void InterpreterVisitor::VisitIfStmt(IfStmt *ifstmt) {
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

void InterpreterConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
    TranslationUnitDecl *decl = Context.getTranslationUnitDecl();
    mEnv.init(decl);
    FunctionDecl *entry = mEnv.getEntry();
    mVisitor.VisitStmt(entry->getBody());
}

std::unique_ptr<clang::ASTConsumer> InterpreterClassAction::CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(
        new InterpreterConsumer(Compiler.getASTContext()));
}
