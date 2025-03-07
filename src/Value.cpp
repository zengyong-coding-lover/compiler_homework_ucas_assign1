#include <Value.h>

Array &Array::operator[](int index) {
    assert(!is_element);
    assert(index < arr.size());
    return arr[index];
}

bool Array::get_is_element() {
    return is_element;
}
bool Array::get_is_pointer() {
    assert(is_element);
    return is_pointer;
}
int Array::get_value() {
    assert(is_element);
    assert(!is_pointer);
    return val;
}
Pointer Array::get_pointer() {
    assert(is_element);
    assert(is_pointer);
    return pointer;
}
int &Array::get_lvalue() {
    assert(is_element);
    assert(!is_pointer);
    return val;
}
Pointer &Array::get_pointer_lval() {
    assert(is_element);
    assert(is_pointer);
    return pointer;
}
void Array::operator=(int val) {
    assert(is_element);
    assert(!is_pointer);
    this->val = val;
}
void Array::operator=(Pointer pointer) {
    assert(is_element);
    assert(is_pointer);
    this->pointer = pointer;
}
VarType Varvalue::get_vartype() {
    return type;
}

bool Varvalue::is_basic_value() {
    return type == Var_Basic_Value;
}

bool Varvalue::is_arr_type() {
    return type == Var_Array;
}

bool Varvalue::is_pointer_type() {
    return type == Var_Pointer;
}
// Array& Varvalue::get_arr() {
//     return arr;
// }
int Varvalue::get_basic_value() {
    assert(type == Var_Basic_Value);
    return val;
}
int &Varvalue::get_basic_lvalue() {
    return val;
}
Pointer Varvalue::get_pointer() {
    return pointer;
}
Pointer &Varvalue::get_pointer_lvalue() {
    return pointer;
}
void Varvalue::set_basic_value(int val) {
    assert(type == Var_Basic_Value);
    this->val = val;
}
void Varvalue::set_pointer(Pointer pointer) {
    assert(type == Var_Pointer);
    assert(this->pointer.get_ref_level() == pointer.get_ref_level());
    this->pointer = pointer;
}
Array &Varvalue::get_arr() {
    assert(type == Var_Array);
    return arr;
}

// int Varvalue::get_array_value(unsigned index) {
//     assert(type == Array_Type);
//     return arr[index];
// }
// void Varvalue::set_array_value(unsigned index, int val) {
//     assert(type == Array_Type);
//     arr[index] = val;
// }
Pointer_Type Pointer::getType() {
    return pointer_type;
}
bool Pointer::is_pointer_pointer() {
    return pointer_type == Pointer_Pointer;
}
bool Pointer::is_basic_val_pointer() {
    return pointer_type == Basic_Value_Pointer;
}
bool Pointer::is_array_pointer() {
    return pointer_type == Array_Pointer;
}
bool Pointer::is_null_pointer() {
    return pointer_type == Null_Pointer;
}
bool Pointer::is_void_pointer() {
    return pointer_type == Void_Pointer;
}
Pointer *Pointer::get_pointer_pointer() {
    assert(is_pointer_pointer());
    return pointer;
}
int *Pointer::get_basic_value_pointer() {
    assert(is_basic_val_pointer());
    return val;
}
Array *Pointer::get_array_pointer() {
    assert(is_array_pointer());
    return arr;
}
void *Pointer::get_void_pointer() {
    assert(is_void_pointer());
    return voi;
}
unsigned Pointer::get_ref_level() {
    return ref_level;
}

void Pointer::set_array_pointer(Array *arr) {
    pointer_type = Array_Pointer;
    this->arr = arr;
}
void Pointer::set_basic_value_pointer(int *val) {
    pointer_type = Basic_Value_Pointer;
    this->val = val;
}
void Pointer::set_pointer_pointer(Pointer *pointer) {
    pointer_type = Pointer_Pointer;
    this->pointer = pointer;
}
void Pointer::set_void_pointer(void *voi) {
    pointer_type = Void_Pointer;
    this->voi = voi;
}
void Pointer::set_ref_level(unsigned ref_level) {
    // assert(pointer_type == Basic_Value_Pointer);
    this->ref_level = ref_level;
}
Nodevalue::Nodevalue(Varvalue &varvalue) {
    if (varvalue.is_basic_value()) {
        set_val(varvalue.get_basic_value());
        set_lval_source(Pointer(&varvalue.get_basic_lvalue()));
        return;
    }
    if (varvalue.is_arr_type()) {
        set_pointer(Pointer(&varvalue.get_arr()));
        return;
    }
    if (varvalue.is_pointer_type()) {
        set_pointer(varvalue.get_pointer());
        set_lval_source(Pointer(&varvalue.get_pointer_lvalue())); // 变量是指针时，左值是指针地址
        return;
    }
}
bool Nodevalue::is_lval() {
    return islval;
}
Pointer Nodevalue::get_lval_source() {
    return source_mem;
}
void Nodevalue::set_lval(Nodevalue rval) {
    assert(islval);
    assert(!source_mem.is_array_pointer()); //左值不会是数组类型
    if (source_mem.is_pointer_pointer()) {
        assert(source_mem.get_ref_level() == rval.get_pointer().get_ref_level() + 1);
        *(source_mem.get_pointer_pointer()) = rval.get_pointer();
        return;
    }
    if (source_mem.is_basic_val_pointer()) {
        assert(source_mem.get_ref_level() == 1);
        *(source_mem.get_basic_value_pointer()) = rval.get_val();
        return;
    }
}
void Nodevalue::set_lval_source(Pointer source) {
    islval = true;
    source_mem = source;
}
bool Nodevalue::get_is_pointer() {
    return is_pointer;
}
int Nodevalue::get_val() {
    assert(!is_pointer);
    return val;
}
Pointer Nodevalue::get_pointer() {
    assert(is_pointer);
    return pointer;
}
void Nodevalue::set_val(int val) {
    is_pointer = false;
    this->val = val;
}
void Nodevalue::set_pointer(Pointer pointer) {
    is_pointer = true;
    this->pointer = pointer;
}
