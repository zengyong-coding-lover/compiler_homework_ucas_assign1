#ifndef __VAR__
#define __VAR__
#include <cassert>
#include <vector>
class Array {
private:
    std::vector<Array> arr;
    int val;
    bool is_element; // 是否是元素
public:
    Array() { }
    Array(int val) {
        is_element = true;
        this->val = val;
    }
    Array(unsigned size, Array arr) {
        for (int i = 0; i < size; i++)
            this->arr.push_back(arr);
    }
    bool get_is_element();
    int get_value();
    int &get_lvalue();
    Array &operator[](int index);
    void operator=(int val);
};
typedef enum {
    Basic_Value_Pointer,
    Array_Pointer,
    Pointer_Pointer,
    Null_Pointer,
} Pointer_Type;
class Pointer {
private:
    Pointer_Type pointer_type;
    union {
        Array *arr;
        int *val;
        Pointer *pointer;
    };

public:
    Pointer() {
    }
    Pointer(Array *arr)
        : pointer_type(Array_Pointer)
        , arr(arr) {
    }
    Pointer(int *val)
        : pointer_type(Basic_Value_Pointer)
        , val(val) {
    }
    Pointer(Pointer *pointer)
        : pointer_type(Pointer_Pointer)
        , pointer(pointer) {
    }
    Pointer_Type getType();
    bool is_pointer_pointer();
    bool is_basic_val_pointer();
    bool is_array_pointer();
    bool is_null_pointer();
    Pointer *get_pointer_pointer();
    int *get_basic_value_pointer();
    Array *get_array_pointer();
    void set_array_pointer(Array *arr);
    void set_basic_value_pointer(int *val);
    void set_pointer_pointer(Pointer *pointer);
};
class Varvalue;
class Nodevalue {
private:
    bool is_pointer;
    union {
        int val;
        Pointer pointer;
    };
    bool islval; // 是否是左值，需要在分析时维护
    Pointer source_mem; // 左值来源（内存中的位置）
public:
    Nodevalue()
        : islval(false) {
    }
    Nodevalue(Pointer pointer)
        : is_pointer(true)
        , islval(false)
        , pointer(pointer) {
    }
    Nodevalue(int val)
        : is_pointer(false)
        , islval(false)
        , val(val) {
    }
    Nodevalue(Varvalue &varvalue);
    bool is_lval();
    Pointer get_lval_source();
    void set_lval_source(Pointer source);
    void set_lval(Nodevalue rval);

    bool get_is_pointer();
    int get_val();
    Pointer get_pointer();
    void set_val(int val);
    void set_pointer(Pointer pointer);

    operator bool() {
        if (is_pointer)
            return pointer.is_null_pointer();
        return val;
    }
};
typedef enum {
    Var_Basic_Value,
    Var_Array,
    Var_Pointer,
} VarType;

class Varvalue {
private:
    VarType type;
    int val;
    Array arr;

public:
    Varvalue() { }
    // Varvalue(int val_size, VarType type) {
    //     if (type == Basic_Value)
    //         val = val_size;
    //     if (type == Array_Type)
    //         arr.resize(val_size);
    // }
    Varvalue(Nodevalue val) // 先默认从Nodevalue到Varvalue 之后有整型转换
        : type(Var_Basic_Value)
        , val(val.get_val()) {
    }
    Varvalue(int val)
        : type(Var_Basic_Value)
        , val(val) {
    }
    Varvalue(Array arr)
        : type(Var_Array)
        , arr(arr) {
    }
    VarType get_vartype();
    bool is_basic_value();
    bool is_arr_type();
    Array &get_arr();
    int get_basic_value();
    int &get_basic_lvalue();
    void set_basic_value(int val);
    // int get_array_value(unsigned index);
    // void set_array_value(unsigned index, int val);
};

#endif