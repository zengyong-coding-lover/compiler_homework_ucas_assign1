#include "Environment.h"

void StackFrame::bindDecl(Decl *decl, const Varvalue &val) {
    mVars[decl] = val;
}

Varvalue &StackFrame::getDeclVal(Decl *decl) {
    assert(mVars.find(decl) != mVars.end());
    return mVars.find(decl)->second;
}

void StackFrame::bindStmt(Stmt *stmt, Nodevalue val) {
    mExprs[stmt] = val;
}

Nodevalue StackFrame::getStmtVal(Stmt *stmt) {
    assert(mExprs.find(stmt) != mExprs.end());
    return mExprs[stmt];
}

void StackFrame::setPC(Stmt *stmt) {
    mPC = stmt;
}

Stmt *StackFrame::getPC() {
    return mPC;
}

void Environment::bind_globals(Decl *decl, const Varvalue &val) {
    mglobals[decl] = val;
}

Varvalue &Environment::getDeclVal_global(Decl *decl) {
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
                bind_globals(vdecl, Varvalue(val));
            }
            else
                bind_globals(vdecl, Varvalue(0));
        }
    }
    mStack.push_back(StackFrame());
}

FunctionDecl *Environment::getEntry() {
    return mEntry;
}

void Environment::intliteral(IntegerLiteral *int_liter) {
    int val = int_liter->getValue().getSExtValue();
    mStack.back().bindStmt(int_liter, Nodevalue(val));
}

void Environment::binop(BinaryOperator *bop) {
    Expr *left = bop->getLHS();
    Expr *right = bop->getRHS();

    if (bop->isAssignmentOp()) {
        Nodevalue rval = mStack.back().getStmtVal(right);
        Nodevalue lval = mStack.back().getStmtVal(left);
        // assert(val.is_lval());
        // set_lval();
        lval.set_lval(rval);
        // mStack.back().bindStmt(left, val); // 更新左值, 个人觉得应该是更新bop才对，但源代码这样写
        // if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(left)) { // 如果左值是变量的引用，更新这个引用
        //     Decl *decl = declexpr->getFoundDecl();
        //     if (is_global_var(decl))
        //         bind_globals(decl, val);
        //     else
        //         mStack.back().bindDecl(decl, val);
        // }
        
        return;
    }

    Nodevalue val1 = mStack.back().getStmtVal(left);
    Nodevalue val2 = mStack.back().getStmtVal(right);
    int re = cal_binary(val1.get_val(), val2.get_val(), bop->getOpcode());
    mStack.back().bindStmt(bop, Nodevalue(re));
}

void Environment::unop(UnaryOperator *uop) {
    Expr *exp = uop->getSubExpr();
    Nodevalue val = mStack.back().getStmtVal(exp);
    int re = cal_unary(val.get_val(), uop->getOpcode());
    mStack.back().bindStmt(uop, Nodevalue(re));
}

void Environment::decl(DeclStmt *declstmt) {
    for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
         it != ie; ++it) {
        Decl *decl = *it;
        if (VarDecl *vardecl = dyn_cast<VarDecl>(decl)) {
            if (Expr *initExpr = vardecl->getInit()) {
                Nodevalue val = mStack.back().getStmtVal(initExpr);
                mStack.back().bindDecl(vardecl, Varvalue(val));
            }
            else
                mStack.back().bindDecl(vardecl, Varvalue(0));
        }
    }
}

void Environment::declref(DeclRefExpr *declref) { // DeclRefExpr是对变量的引用
    mStack.back().setPC(declref);
    if (declref->getType()->isIntegerType()) {
        Decl *decl = declref->getFoundDecl();
        // decl->dump();
        if (is_global_var(decl))
            mStack.back().bindStmt(declref, Nodevalue(getDeclVal_global(decl)));
        else
            mStack.back().bindStmt(declref, Nodevalue(mStack.back().getDeclVal(decl)));
    }
}

void Environment::cast(CastExpr *castexpr) {
    mStack.back().setPC(castexpr);
    if (castexpr->getType()->isIntegerType()) {
        Expr *expr = castexpr->getSubExpr();
        Nodevalue val = mStack.back().getStmtVal(expr);
        mStack.back().bindStmt(castexpr, val);
    }
}

Stmt *Environment::iff(IfStmt *ifstmt) {
    mStack.back().setPC(ifstmt);
    Expr *cond = ifstmt->getCond();
    Stmt *then = ifstmt->getThen();
    Stmt *elif = ifstmt->getElse();
    Nodevalue val = mStack.back().getStmtVal(cond);
    if (val) {
        return then;
    }
    else {
        if (elif)
            return elif;
        return nullptr;
    }
}

// 返回条件是否为真，相当于跳转指令
bool Environment::_while_(WhileStmt *whilestmt) {
    mStack.back().setPC(whilestmt);
    Expr *cond = whilestmt->getCond();
    Nodevalue val = mStack.back().getStmtVal(cond);
    return val;
}

bool Environment::_for_(ForStmt *forstmt) {
    mStack.back().setPC(forstmt);
    Expr *cond = forstmt->getCond();
    Nodevalue val = mStack.back().getStmtVal(cond);
    return val;
}

// 递归查找数组定义的 Decl
Decl *findArrayDecl(Expr *expr) {
    expr = expr->IgnoreParenImpCasts(); // 忽略括号和隐式转换

    if (clang::ArraySubscriptExpr *arrayExpr = llvm::dyn_cast<clang::ArraySubscriptExpr>(expr)) {
        // 如果 Base 是另一个数组访问，递归处理
        return findArrayDecl(arrayExpr->getBase());
    }

    if (DeclRefExpr *declRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
        // 找到 DeclRefExpr，返回对应的 Decl
        return declRefExpr->getFoundDecl();
    }
    return nullptr; // 没有找到 Decl
}

void Environment::call(CallExpr *callexpr) {
    mStack.back().setPC(callexpr);
    FunctionDecl *callee = callexpr->getDirectCallee();
    if (callee == mInput) {
        llvm::errs() << "Please Input an Integer Value : ";
        int input;
        scanf("%d", &input);
        mStack.back().bindStmt(callexpr, Nodevalue(input));
    }
    else if (callee == mOutput) {
        Expr *decl = callexpr->getArg(0);
        Nodevalue val = mStack.back().getStmtVal(decl);
        llvm::errs() << val.get_val();
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
            Nodevalue val = mStack[current_frame_index].getStmtVal(arg);

            assert(!val.get_is_pointer());
            mStack.back().bindDecl(params[i], Varvalue(val));
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
