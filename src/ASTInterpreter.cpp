#include "ASTInterpreter.h"
void InterpreterVisitor::_VisitExpr_(Expr *exp) {
    if (BinaryOperator *bop = llvm::dyn_cast<BinaryOperator>(exp))
        VisitBinaryOperator(bop);
    else if (DeclRefExpr *declRef = llvm::dyn_cast<DeclRefExpr>(exp))
        VisitDeclRefExpr(declRef);
    else if (CallExpr *call = llvm::dyn_cast<CallExpr>(exp))
        VisitCallExpr(call);
    else if (CastExpr *cast = llvm::dyn_cast<CastExpr>(exp))
        VisitCastExpr(cast);
    else if (ArraySubscriptExpr *array = llvm::dyn_cast<ArraySubscriptExpr>(exp))
        VisitArraySubscriptExpr(array);
    else
        // If it's an expression type that we haven't handled explicitly, visit it as a generic statement
        VisitStmt(exp);
}

void InterpreterVisitor::_VisitStmt_(Stmt *stmt) {
    if (!stmt) return;
    if (Expr *exp = dyn_cast<Expr>(stmt)) {
        // stmt 可能是expr,主要解决 if () exp else exp / while / for 的这种情形
        _VisitExpr_(exp);
        return;
    }
    if (ReturnStmt *retstmt = dyn_cast<ReturnStmt>(stmt))
        VisitReturnStmt(retstmt);
    else if (DeclStmt *declstmt = dyn_cast<DeclStmt>(stmt))
        VisitDeclStmt(declstmt);
    else if (IfStmt *ifstmt = dyn_cast<IfStmt>(stmt))
        VisitIfStmt(ifstmt);
    else if (WhileStmt *whilestmt = dyn_cast<WhileStmt>(stmt))
        VisitWhileStmt(whilestmt);
    else
        VisitStmt(stmt);
}

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
void InterpreterVisitor::VisitCStyleCastExpr(CStyleCastExpr *expr) {
    VisitStmt(expr);
    mEnv->cstylecast(expr);
}
void InterpreterVisitor::VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *expr) {
    VisitStmt(expr);
    mEnv->unary_trait(expr);
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
    _VisitExpr_(cond);

    Stmt *next = mEnv->iff(ifstmt);
    // next->dump();
    if (next) {
        _VisitStmt_(next);
    }
}

void InterpreterVisitor::VisitWhileStmt(WhileStmt *whilestmt) {
    Expr *cond = whilestmt->getCond();
    Stmt *body = whilestmt->getBody();
    _VisitExpr_(cond);
    while (mEnv->_while_(whilestmt)) {
        _VisitStmt_(body);
        _VisitExpr_(cond);
    }
}

void InterpreterVisitor::VisitForStmt(ForStmt *forstmt) {
    Stmt *init = forstmt->getInit();
    Expr *cond = forstmt->getCond();
    Expr *inc = forstmt->getInc();
    Stmt *body = forstmt->getBody();

    for (_VisitStmt_(init), _VisitExpr_(cond); mEnv->_for_(forstmt); _VisitStmt_(body), _VisitExpr_(inc), _VisitExpr_(cond))
        ;
}

void InterpreterVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr *array) {
    VisitStmt(array);
    mEnv->array(array);
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
