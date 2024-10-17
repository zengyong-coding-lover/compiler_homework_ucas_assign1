#include "Environment.h"

void StackFrame::bindDecl(Decl *decl, int val) {
    mVars[decl] = val;
}

int StackFrame::getDeclVal(Decl *decl) {
    assert(mVars.find(decl) != mVars.end());
    return mVars.find(decl)->second;
}

void StackFrame::bindStmt(Stmt *stmt, int val) {
    mExprs[stmt] = val;
}

int StackFrame::getStmtVal(Stmt *stmt) {
    assert(mExprs.find(stmt) != mExprs.end());
    return mExprs[stmt];
}

void StackFrame::setPC(Stmt *stmt) {
    mPC = stmt;
}

Stmt *StackFrame::getPC() {
    return mPC;
}

void Environment::bind_globals(Decl *decl, int val) {
    mglobals[decl] = val;
}

int Environment::getDeclVal_global(Decl *decl) {
    assert(mglobals.find(decl) != mglobals.end());
    return mglobals[decl];
}

void Environment::init(TranslationUnitDecl *unit) {
    for (TranslationUnitDecl::decl_iterator i = unit->decls_begin(), e = unit->decls_end(); i != e; ++i) {
        if (FunctionDecl *fdecl = dyn_cast<FunctionDecl>(*i)) {
            if (fdecl->getName().equals("FREE")) mFree = fdecl;
            else if (fdecl->getName().equals("MALLOC"))
                mMalloc = fdecl;
            else if (fdecl->getName().equals("GET"))
                mInput = fdecl;
            else if (fdecl->getName().equals("PRINT"))
                mOutput = fdecl;
            else if (fdecl->getName().equals("main"))
                mEntry = fdecl;
        }
        // 初始化全局变量
        if (VarDecl *vdecl = dyn_cast<VarDecl>(*i)) {
            if (Expr *initExpr = vdecl->getInit()) {
                int val = caluate_exp(initExpr);
                bind_globals(vdecl, val);
            }
            else
                bind_globals(vdecl, 0);
        }
    }
    mStack.push_back(StackFrame());
}

FunctionDecl *Environment::getEntry() {
    return mEntry;
}

void Environment::intliteral(IntegerLiteral *int_liter) {
    int val = int_liter->getValue().getSExtValue();
    mStack.back().bindStmt(int_liter, val);
}

void Environment::binop(BinaryOperator *bop) {
    Expr *left = bop->getLHS();
    Expr *right = bop->getRHS();

    if (bop->isAssignmentOp()) {
        int val = mStack.back().getStmtVal(right);
        mStack.back().bindStmt(left, val); // 更新左值, 个人觉得应该是更新bop才对，但源代码这样写
        if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(left)) { // 如果左值是变量的引用，更新这个引用
            Decl *decl = declexpr->getFoundDecl();
            if (is_global_var(decl))
                bind_globals(decl, val);
            else
                mStack.back().bindDecl(decl, val);
        }
        return;
    }

    int val1 = mStack.back().getStmtVal(left);
    int val2 = mStack.back().getStmtVal(right);
    int re = cal_binary(val1, val2, bop->getOpcode());
    mStack.back().bindStmt(bop, re);
}

void Environment::unop(UnaryOperator *uop) {
    Expr *exp = uop->getSubExpr();
    int val = mStack.back().getStmtVal(exp);
    int re = cal_unary(val, uop->getOpcode());
    mStack.back().bindStmt(uop, re);
}

void Environment::decl(DeclStmt *declstmt) {
    for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
         it != ie; ++it) {
        Decl *decl = *it;
        if (VarDecl *vardecl = dyn_cast<VarDecl>(decl)) {
            if (Expr *initExpr = vardecl->getInit()) {
                // Expr::EvalResult result;
                // assert(initExpr->EvaluateAsRValue(result, ConstExprUsage Usage, const ASTContext &Ctx))
                // initExpr->dump();
                // assert(initExpr->)
                int val = mStack.back().getStmtVal(initExpr);
                mStack.back().bindDecl(vardecl, val);
            }
            else
                mStack.back().bindDecl(vardecl, 0);
        }
    }
}

void Environment::declref(DeclRefExpr *declref) { // DeclRefExpr是对变量的引用
    mStack.back().setPC(declref);
    if (declref->getType()->isIntegerType()) {
        Decl *decl = declref->getFoundDecl();
        // decl->dump();
        int val;
        if (is_global_var(decl))
            val = getDeclVal_global(decl);
        else
            val = mStack.back().getDeclVal(decl);
        mStack.back().bindStmt(declref, val);
    }
}

void Environment::cast(CastExpr *castexpr) {
    mStack.back().setPC(castexpr);
    if (castexpr->getType()->isIntegerType()) {
        Expr *expr = castexpr->getSubExpr();
        int val = mStack.back().getStmtVal(expr);
        mStack.back().bindStmt(castexpr, val);
    }
}

Stmt *Environment::iff(IfStmt *ifstmt) {
    mStack.back().setPC(ifstmt);
    Expr *cond = ifstmt->getCond();
    Stmt *then = ifstmt->getThen();
    Stmt *elif = ifstmt->getElse();
    int val = mStack.back().getStmtVal(cond);
    if (val) {
        return then;
    }
    else {
        if (elif)
            return elif;
        return nullptr;
    }
}

void Environment::call(CallExpr *callexpr) {
    mStack.back().setPC(callexpr);
    int val = 0;
    FunctionDecl *callee = callexpr->getDirectCallee();
    if (callee == mInput) {
        llvm::errs() << "Please Input an Integer Value : ";
        scanf("%d", &val);

        mStack.back().bindStmt(callexpr, val);
    }
    else if (callee == mOutput) {
        Expr *decl = callexpr->getArg(0);
        val = mStack.back().getStmtVal(decl);
        llvm::errs() << val;
    }
    else {
        /// You could add your code here for Function call Return
        // 准备参数
        unsigned current_frame_index = mStack.size() - 1;
        mStack.push_back(StackFrame());

        //push_back可能重新分配内存，这时候StackFrame和原来不一样，不能记录绝对地址
        unsigned numArgs = callexpr->getNumArgs();

        const auto &params = callee->parameters();
        unsigned numParams = params.size();

        assert(numArgs == numParams);

        for (unsigned i = 0; i < numArgs; i++) {
            Expr *arg = callexpr->getArg(i);
            val = mStack[current_frame_index].getStmtVal(arg);

            mStack.back().bindDecl(params[i], val);
        }
    }
}

void Environment::ret(ReturnStmt *retstmt) {
    mStack.back().setPC(retstmt);
    Expr *expr = retstmt->getRetValue();

    ret_val = mStack.back().getStmtVal(expr);
}

void Environment::finish_call(CallExpr *callexpr) {
    FunctionDecl *callee = callexpr->getDirectCallee();
    if (callee == mInput) return;
    if (callee == mOutput) return;
    mStack.pop_back();
    mStack.back().bindStmt(callexpr, ret_val);
}
