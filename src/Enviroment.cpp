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
        lval.set_lval(rval);

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
    if (uop->getOpcode() == clang::UO_Deref) {
        Pointer pointer = val.get_pointer();
        assert(!pointer.is_array_pointer());
        assert(!pointer.is_void_pointer());
        Nodevalue nodeval;
        if (pointer.is_basic_val_pointer()) {
            int val = *(pointer.get_basic_value_pointer());
            nodeval.set_val(val);
        }
        if (pointer.is_pointer_pointer()) {
            Pointer new_pointer = *(pointer.get_pointer_pointer());
            nodeval.set_pointer(new_pointer);
        }
        nodeval.set_lval_source(pointer);
        mStack.back().bindStmt(uop, nodeval);
        return;
    }
    int re = cal_unary(val.get_val(), uop->getOpcode());
    mStack.back().bindStmt(uop, Nodevalue(re));
}

static Array InitArr(const clang::ArrayType *arrType) {
    if (!arrType)
        return Array(0);
    const clang::ConstantArrayType *constArr = llvm::dyn_cast<clang::ConstantArrayType>(arrType);
    unsigned size = constArr->getSize().getZExtValue();
    const clang::ArrayType *nextArrType = constArr->getElementType()->getAsArrayTypeUnsafe();
    return Array(size, InitArr(nextArrType));
}

static Pointer InitPointer(VarDecl *decl) {
    const Type *Ty = decl->getType().getTypePtr();

    unsigned ref_level = 0;
    while (Ty->isPointerType()) {
        ref_level++;
        Ty = Ty->getPointeeType().getTypePtr();
    }
    if (ref_level == 1)
        return Pointer(Basic_Value_Pointer, ref_level);
    else
        return Pointer(Pointer_Pointer, ref_level);
}
void Environment::decl(DeclStmt *declstmt) {
    for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
         it != ie; ++it) {
        Decl *decl = *it;
        if (VarDecl *vardecl = dyn_cast<VarDecl>(decl)) {
            if (const clang::ArrayType *arrType = vardecl->getType()->getAsArrayTypeUnsafe()) {
                Array arr = InitArr(arrType);
                mStack.back().bindDecl(vardecl, Varvalue(arr));
                continue;
            }
            if (vardecl->getType()->isPointerType()) { // 指针变量
                Pointer pointer = InitPointer(vardecl);
                mStack.back().bindDecl(vardecl, Varvalue(pointer));
                continue;
            }
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
        if (is_global_var(decl))
            mStack.back().bindStmt(declref, Nodevalue(getDeclVal_global(decl)));
        else
            mStack.back().bindStmt(declref, Nodevalue(mStack.back().getDeclVal(decl)));
        return;
    }
    if (declref->getType()->isFunctionProtoType()) return;
    if (declref->getType()->isArrayType()) {
        Decl *decl = declref->getFoundDecl();
        if (is_global_var(decl))
            mStack.back().bindStmt(declref, Nodevalue(getDeclVal_global(decl)));
        else
            mStack.back().bindStmt(declref, Nodevalue(mStack.back().getDeclVal(decl)));
    }
    if (declref->getType()->isPointerType()) {
        Decl *decl = declref->getFoundDecl();
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
        return;
    }
    if (castexpr->getType()->isPointerType()) { // 不知道怎么表示数组类型，只用了指针指向数组的位置
        Expr *expr = castexpr->getSubExpr();
        if (expr->getType()->isFunctionProtoType()) return;
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
void Environment::cstylecast(CStyleCastExpr *expr) {
    mStack.back().setPC(expr);
    const Type *Ty = expr->getType().getTypePtr();
    unsigned ref_level = 0;
    while (Ty->isPointerType()) {
        ref_level++;
        Ty = Ty->getPointeeType().getTypePtr();
    }
    assert(ref_level > 0);
    Expr *subexpr = expr->getSubExpr();
    Pointer pointer = mStack.back().getStmtVal(subexpr).get_pointer();
    Pointer new_pointer;
    if (ref_level == 1)
        new_pointer = Pointer((int *) pointer.get_void_pointer());
    else
        new_pointer = Pointer((Pointer *) pointer.get_void_pointer());
    new_pointer.set_ref_level(ref_level);
    mStack.back().bindStmt(expr, Nodevalue(new_pointer));
}
void Environment::array(ArraySubscriptExpr *arr) {
    mStack.back().setPC(arr);
    Expr *astbase = arr->getBase();
    Expr *astindex = arr->getIdx();
    Nodevalue nodebase = mStack.back().getStmtVal(astbase);
    Nodevalue nodeindex = mStack.back().getStmtVal(astindex);
    Array *arrbase = nodebase.get_pointer().get_array_pointer();
    unsigned index = nodeindex.get_val();
    Array *arrnow = &((*arrbase)[index]);
    if (arrnow->get_is_element()) { // 索引到最后一个变成左值
        Nodevalue nodenow = Nodevalue(arrnow->get_value());
        nodenow.set_lval_source(Pointer(&arrnow->get_lvalue()));
        mStack.back().bindStmt(arr, nodenow);
    }
    else {
        mStack.back().bindStmt(arr, Nodevalue(arrnow));
    }
}
void Environment::unary_trait(UnaryExprOrTypeTraitExpr *expr) {
    mStack.back().setPC(expr);
    assert(expr->getKind() == clang::UETT_SizeOf);
    if (expr->getTypeOfArgument()->isIntegerType()) {
        mStack.back().bindStmt(expr, Nodevalue(4));
        return;
    }
    if (expr->getTypeOfArgument()->isPointerType()) {
        mStack.back().bindStmt(expr, Nodevalue(8));
        return;
    }
    assert(0);
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
    else if (callee == mMalloc) {
        Nodevalue val = mStack.back().getStmtVal(callexpr->getArg(0));
        void *mem = malloc(val.get_val());
        mStack.back().bindStmt(callexpr, Nodevalue(Pointer(mem)));
    }
    else if (callee == mFree) {
        Nodevalue val = mStack.back().getStmtVal(callexpr->getArg(0));
        Pointer pointer = val.get_pointer();
        assert(!pointer.is_array_pointer());
        if (pointer.is_basic_val_pointer())
            free(pointer.get_basic_value_pointer());
        if (pointer.is_void_pointer())
            free(pointer.get_void_pointer());
        if (pointer.is_pointer_pointer())
            free(pointer.get_pointer_pointer());
    }
    else {
        /// You could add your code here for Function call Return
        // 准备参数
        unsigned current_frame_index = mStack.size() - 1;
        ret_val.set_val(0);
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
    mStack.pop_back();
}

void Environment::finish_call(CallExpr *callexpr) {
    FunctionDecl *callee = callexpr->getDirectCallee();
    if (callee == mInput) return;
    if (callee == mOutput) return;
    if (callee == mFree) return;
    if (callee == mMalloc) return;
    if (callee->getReturnType()->isVoidType())
        return;
    mStack.back().bindStmt(callexpr, ret_val);
}
