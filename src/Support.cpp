#include "Support.h"
int cal_binary(int val1, int val2, BinaryOperatorKind kind) {
    switch (kind) {
    case BO_Add:
        return val1 + val2;
    case BO_Sub:
        return val1 - val2;
    case BO_Mul:
        return val1 * val2;
    case BO_Div:
        assert(val2 != 0);
        return val1 / val2;
    case BO_Rem:
        return val1 % val2;
    case BO_And:
        return val1 & val2;
    case BO_Or:
        return val1 | val2;
    case BO_Xor:
        return val1 ^ val2;
    case BO_Shl:
        return val1 << val2;
    case BO_Shr:
        return val1 >> val2;
    case BO_LAnd:
        return val1 && val2;
    case BO_LOr:
        return val1 || val2;
    case BO_Comma:
        return val1;
    case BO_LE:
        return val1 <= val2;
    case BO_LT:
        return val1 < val2;
    case BO_GT:
        return val1 > val2;
    case BO_GE:
        return val1 >= val2;
    case BO_NE:
        return val1 != val2;
    case BO_EQ:
        return val1 == val2;
    default:
        assert(0);
    }
}
int cal_unary(int val, UnaryOperatorKind kind) {
    switch (kind) {
    case UO_Plus:
        return val;
    case UO_Minus:
        return -val;
    case UO_Not:
        return ~val;
    case UO_LNot:
        return !val;
    default:
        assert(0);
    }
}

int caluate_exp(Expr *exp) { // 简单的对全局变量初始化的常量折叠
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

bool is_global_var(Decl *decl) {
    if (VarDecl *vdecl = dyn_cast<VarDecl>(decl)) {
        return vdecl->hasGlobalStorage() && vdecl->isFileVarDecl();
    }
    return false;
}