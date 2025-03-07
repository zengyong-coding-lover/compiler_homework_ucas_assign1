//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#ifndef __Environment__
#define __Environment__
#include "Support.h"
#include "Value.h"
using namespace clang;

class StackFrame {
    /// StackFrame maps Variable Declaration to Value
    /// Which are either integer or addresses (also represented using an Integer value)
    std::map<Decl *, Varvalue> mVars;
    std::map<Stmt *, Nodevalue> mExprs;
    /// The current stmt
    Stmt *mPC;

public:
    StackFrame()
        : mVars()
        , mExprs()
        , mPC() {
    }

    void bindDecl(Decl *decl, const Varvalue &val);
    Varvalue &getDeclVal(Decl *decl);
    void bindStmt(Stmt *stmt, Nodevalue val);
    Nodevalue getStmtVal(Stmt *stmt);
    void setPC(Stmt *stmt);
    Stmt *getPC();
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

    std::map<Decl *, Varvalue> mglobals;

    Nodevalue ret_val;

    FunctionDecl *mFree; /// Declartions to the built-in functions
    FunctionDecl *mMalloc;
    FunctionDecl *mInput;
    FunctionDecl *mOutput;

    FunctionDecl *mEntry;

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
    void bind_globals(Decl *decl, const Varvalue &val);
    Varvalue &getDeclVal_global(Decl *decl);
    /// Initialize the Environment
    void init(TranslationUnitDecl *unit);

    FunctionDecl *getEntry();

    void intliteral(IntegerLiteral *int_liter);
    /// !TODO Support comparison operation
    void binop(BinaryOperator *bop);
    void unop(UnaryOperator *uop);
    void decl(DeclStmt *declstmt);
    void paren(ParenExpr *parent);
    // 继承关系：
    //    Stmt
    // └── Expr
    //     └── DeclRefExpr
    void declref(DeclRefExpr *declref);
    void cast(CastExpr *castexpr);

    // 返回下一步执行stmt
    Stmt *iff(IfStmt *ifstmt);
    // 返回是否继续执行
    bool _while_(WhileStmt *whilestmt);
    bool _for_(ForStmt *forstmt);

    void cstylecast(CStyleCastExpr* expr);
    void array(ArraySubscriptExpr *arr);
    void unary_trait(UnaryExprOrTypeTraitExpr *expr);
    /// !TODO Support Function Call
    void call(CallExpr *callexpr);

    void ret(ReturnStmt *ret);

    void finish_call(CallExpr *callexpr);
};

#endif