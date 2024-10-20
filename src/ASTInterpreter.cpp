#include "ASTInterpreter.h"
bool InterpreterVisitor::_VisitExpr_(Expr *exp) {
    if (BinaryOperator *bop = llvm::dyn_cast<BinaryOperator>(exp))
        VisitBinaryOperator(bop);
    else if (UnaryOperator *uop = llvm::dyn_cast<UnaryOperator>(exp))
        VisitUnaryOperator(uop);
    else if (DeclRefExpr *declRef = llvm::dyn_cast<DeclRefExpr>(exp))
        VisitDeclRefExpr(declRef);
    else if (CallExpr *call = llvm::dyn_cast<CallExpr>(exp))
        VisitCallExpr(call);
    else if (CStyleCastExpr *cstylecast = llvm::dyn_cast<CStyleCastExpr>(exp))
        VisitCStyleCastExpr(cstylecast);
    else if (CastExpr *cast = llvm::dyn_cast<CastExpr>(exp))
        VisitCastExpr(cast);
    else if (ArraySubscriptExpr *array = llvm::dyn_cast<ArraySubscriptExpr>(exp))
        VisitArraySubscriptExpr(array);
    else if (IntegerLiteral *intliteral = llvm::dyn_cast<IntegerLiteral>(exp))
        VisitIntegerLiteral(intliteral);
    else if (UnaryExprOrTypeTraitExpr *unarytrait = llvm::dyn_cast<UnaryExprOrTypeTraitExpr>(exp))
        VisitUnaryExprOrTypeTraitExpr(unarytrait);
    else if (ParenExpr *parenexp = llvm::dyn_cast<ParenExpr>(exp))
        VisitParenExpr(parenexp);
    else
        // If it's an expression type that we haven't handled explicitly, visit it as a generic statement
        return VisitStmt(exp);
    return true;
}

bool InterpreterVisitor::_VisitStmt_(Stmt *stmt) {
    if (!stmt) return true;
    if (Expr *exp = dyn_cast<Expr>(stmt)) {
        // stmt 可能是expr,主要解决 if () exp else exp / while / for 的这种情形
        _VisitExpr_(exp);
        return true;
    }
    if (ReturnStmt *retstmt = dyn_cast<ReturnStmt>(stmt)) {
        return VisitReturnStmt(retstmt);
    }
    else if (DeclStmt *declstmt = dyn_cast<DeclStmt>(stmt))
        return VisitDeclStmt(declstmt);
    else if (IfStmt *ifstmt = dyn_cast<IfStmt>(stmt))
        return VisitIfStmt(ifstmt);
    else if (WhileStmt *whilestmt = dyn_cast<WhileStmt>(stmt))
        return VisitWhileStmt(whilestmt);
    else if (ForStmt *forstmt = dyn_cast<ForStmt>(stmt))
        return VisitForStmt(forstmt);
    else
        return VisitStmt(stmt);
    return true;
}
bool InterpreterVisitor::VisitStmt(Stmt *stmt) {
    for (auto *SubStmt : stmt->children()) {
        if (SubStmt)
            if (!_VisitStmt_(SubStmt)) {
                return false;
            }
    }
    return true;
}
bool InterpreterVisitor::VisitIntegerLiteral(IntegerLiteral *int_literal) {
    mEnv->intliteral(int_literal);
    return true;
}

bool InterpreterVisitor::VisitBinaryOperator(BinaryOperator *bop) {
    if (!VisitStmt(bop)) return false;
    mEnv->binop(bop);
    return true;
}
bool InterpreterVisitor::VisitUnaryOperator(UnaryOperator *uop) {
    if (!VisitStmt(uop)) return false;
    mEnv->unop(uop);
    return true;
}
bool InterpreterVisitor::VisitDeclRefExpr(DeclRefExpr *expr) {
    if (!VisitStmt(expr)) return false;
    mEnv->declref(expr);
    return true;
}
bool InterpreterVisitor::VisitCastExpr(CastExpr *expr) {
    if (!VisitStmt(expr)) return false;
    mEnv->cast(expr);
    return true;
}
bool InterpreterVisitor::VisitCStyleCastExpr(CStyleCastExpr *expr) {
    if (!VisitStmt(expr)) return false;
    mEnv->cstylecast(expr);
    return true;
}
bool InterpreterVisitor::VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *expr) {
    if (!VisitStmt(expr)) return false;
    mEnv->unary_trait(expr);
    return true;
}
bool InterpreterVisitor::VisitParenExpr(ParenExpr *parent) {
    if (!VisitStmt(parent)) return false;
    mEnv->paren(parent);
    return true;
}

bool InterpreterVisitor::VisitCallExpr(CallExpr *call) {
    if (!VisitStmt(call)) return false;
    mEnv->call(call);

    FunctionDecl *callee = call->getDirectCallee();
    if (Stmt *body = callee->getBody()) {
        assert(!VisitStmt(body)); // 先默认函数调用一定有return
    }

    mEnv->finish_call(call);
    return true;
}
bool InterpreterVisitor::VisitReturnStmt(ReturnStmt *retstmt) {
    if (!VisitStmt(retstmt)) return false;
    mEnv->ret(retstmt);
    return false;
}
bool InterpreterVisitor::VisitDeclStmt(DeclStmt *declstmt) {
    if (!VisitStmt(declstmt)) return false;
    mEnv->decl(declstmt);
    return true;
}
bool InterpreterVisitor::VisitIfStmt(IfStmt *ifstmt) {
    Expr *cond = ifstmt->getCond();
    _VisitExpr_(cond);

    Stmt *next = mEnv->iff(ifstmt);
    if (next) {
        if (!_VisitStmt_(next)) return false;
    }
    return true;
}

bool InterpreterVisitor::VisitWhileStmt(WhileStmt *whilestmt) {
    Expr *cond = whilestmt->getCond();
    Stmt *body = whilestmt->getBody();
    _VisitExpr_(cond);
    while (mEnv->_while_(whilestmt)) {
        if (!_VisitStmt_(body)) return false;
        _VisitExpr_(cond);
    }
    return true;
}

bool InterpreterVisitor::VisitForStmt(ForStmt *forstmt) {
    Stmt *init = forstmt->getInit();
    Expr *cond = forstmt->getCond();
    Expr *inc = forstmt->getInc();
    Stmt *body = forstmt->getBody();

    if (!_VisitStmt_(init)) return false;
    _VisitExpr_(cond);
    while (mEnv->_for_(forstmt)) {
        if (!_VisitStmt_(body)) return false;
        _VisitExpr_(inc);
        _VisitExpr_(cond);
    }
    return true;
    // for (_VisitStmt_(init), _VisitExpr_(cond); mEnv->_for_(forstmt); _VisitStmt_(body), _VisitExpr_(inc), _VisitExpr_(cond))
    //     ;
}

bool InterpreterVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr *array) {
    if (!VisitStmt(array)) return false;
    mEnv->array(array);
    return true;
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
