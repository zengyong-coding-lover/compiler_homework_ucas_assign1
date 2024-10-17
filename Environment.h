//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include <stdio.h>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

class StackFrame {
    /// StackFrame maps Variable Declaration to Value
    /// Which are either integer or addresses (also represented using an Integer value)
    std::map<Decl *, int> mVars;
    std::map<Stmt *, int> mExprs;
    /// The current stmt
    Stmt *mPC;

public:
    StackFrame()
        : mVars()
        , mExprs()
        , mPC() {
    }

    void bindDecl(Decl *decl, int val) {
        mVars[decl] = val;
    }
    int getDeclVal(Decl *decl) {
        assert(mVars.find(decl) != mVars.end());
        return mVars.find(decl)->second;
    }
    void bindStmt(Stmt *stmt, int val) {
        mExprs[stmt] = val;
    }
    int getStmtVal(Stmt *stmt) {
        assert(mExprs.find(stmt) != mExprs.end());
        return mExprs[stmt];
    }
    void setPC(Stmt *stmt) {
        mPC = stmt;
    }
    Stmt *getPC() {
        return mPC;
    }
};

/// Heap maps address to a value
/*
class Heap {
public:
   int Malloc(int size) ;
   void Free (int addr) ;
   void Update(int addr, int val) ;
   int get(int addr);
};
*/

class Environment {
    std::vector<StackFrame> mStack;

    std::map<Decl *, int> mglobals;

    int ret_val;

    FunctionDecl *mFree; /// Declartions to the built-in functions
    FunctionDecl *mMalloc;
    FunctionDecl *mInput;
    FunctionDecl *mOutput;

    FunctionDecl *mEntry;

    static int cal_binary(int val1, int val2, BinaryOperatorKind kind) {
        switch (kind) {
        case clang::BO_Add:
            return val1 + val2;
        case clang::BO_Sub:
            return val1 - val2;
        case clang::BO_Mul:
            return val1 * val2;
        case clang::BO_Div:
            assert(val2 != 0);
            return val1 / val2;
        case clang::BO_Rem:
            return val1 % val2;
        case clang::BO_And:
            return val1 & val2;
        case clang::BO_Or:
            return val1 | val2;
        case clang::BO_Xor:
            return val1 ^ val2;
        case clang::BO_Shl:
            return val1 << val2;
        case clang::BO_Shr:
            return val1 >> val2;
        case clang::BO_LAnd:
            return val1 && val2;
        case clang::BO_LOr:
            return val1 || val2;
        case clang::BO_Comma:
            return val1;
        case clang::BO_LE:
            return val1 <= val2;
        case clang::BO_LT:
            return val1 < val2;
        case clang::BO_GT:
            return val1 > val2;
        case clang::BO_GE:
            return val1 >= val2;
        case clang::BO_NE:
            return val1 != val2;
        case clang::BO_EQ:
            return val1 == val2;
        default:
            assert(0);
        }
    }
    static int cal_unary(int val, UnaryOperatorKind kind) {
        switch (kind) {
        case clang::UO_Plus:
            return val;
        case clang::UO_Minus:
            return -val;
        case clang::UO_Not:
            return ~val;
        case clang::UO_LNot:
            return !val;
        default:
            assert(0);
        }
    }
    static int caluate_exp(Expr *exp) { // 简单的对全局变量初始化的常量折叠
        if (BinaryOperator *binary = dyn_cast<BinaryOperator>(exp)) {
            int val1 = caluate_exp(binary->getLHS());
            int val2 = caluate_exp(binary->getRHS());
            return cal_binary(val1, val2, binary->getOpcode());
        }
        if (UnaryOperator *unary = dyn_cast<UnaryOperator>(exp)) {
            int val = caluate_exp(unary->getSubExpr());
            return cal_unary(val, unary->getOpcode());
        }
        if (IntegerLiteral *intliteral = dyn_cast<IntegerLiteral>(exp)) {
            return intliteral->getValue().getSExtValue();
        }
        assert(0);
    }
    static bool is_global_var(Decl *decl) {
        if (VarDecl *vdecl = dyn_cast<VarDecl>(decl)) {
            return vdecl->hasGlobalStorage() && vdecl->isFileVarDecl();
        }
        return false;
    }
    void bind_globals(Decl *decl, int val) {
        mglobals[decl] = val;
    }
    int getDeclVal_global(Decl *decl) {
        assert(mglobals.find(decl) != mglobals.end());
        return mglobals[decl];
    }

public:
    /// Get the declartions to the built-in functions
    Environment()
        : mStack()
        , mFree(NULL)
        , mMalloc(NULL)
        , mInput(NULL)
        , mOutput(NULL)
        , mEntry(NULL) {
    }

    /// Initialize the Environment
    void init(TranslationUnitDecl *unit) {
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

    FunctionDecl *getEntry() {
        return mEntry;
    }

    void intliteral(IntegerLiteral *int_liter) {
        int val = int_liter->getValue().getSExtValue();
        mStack.back().bindStmt(int_liter, val);
    }
    /// !TODO Support comparison operation
    void binop(BinaryOperator *bop) {
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
    void unop(UnaryOperator *uop) {
        Expr *exp = uop->getSubExpr();
        int val = mStack.back().getStmtVal(exp);
        int re = cal_unary(val, uop->getOpcode());
        mStack.back().bindStmt(uop, re);
    }
    void decl(DeclStmt *declstmt) {
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
    // 继承关系：
    //    Stmt
    // └── Expr
    //     └── DeclRefExpr
    void declref(DeclRefExpr *declref) { // DeclRefExpr是对变量的引用
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

    void cast(CastExpr *castexpr) {
        mStack.back().setPC(castexpr);
        if (castexpr->getType()->isIntegerType()) {
            Expr *expr = castexpr->getSubExpr();
            int val = mStack.back().getStmtVal(expr);
            mStack.back().bindStmt(castexpr, val);
        }
    }

    // 返回下一步执行stmt
    Stmt *iff(IfStmt *ifstmt) {
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
    /// !TODO Support Function Call
    void call(CallExpr *callexpr) {
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

    void ret(ReturnStmt *ret) {
        mStack.back().setPC(ret);
        Expr *expr = ret->getRetValue();

        ret_val = mStack.back().getStmtVal(expr);
    }

    void finish_call(CallExpr *callexpr) {
        FunctionDecl *callee = callexpr->getDirectCallee();
        if (callee == mInput) return;
        if (callee == mOutput) return;
        mStack.pop_back();
        mStack.back().bindStmt(callexpr, ret_val);
    }
};
