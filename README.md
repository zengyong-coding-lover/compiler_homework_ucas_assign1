# clang 前端模拟器
这是国科大编译课程作业，目标是实现一个clang前端的模拟器，并通过测试，测试包括对全局变量、函数调用、分支及循环语句、一维数组、动态内存分配、高维指针的功能。

仓库地址：https://github.com/zengyong-coding-lover/compiler_homework_ucas.git
## 安装方式
```
mkdir build && cd build
cmake .. -DClang_DIR=/usr/lib/cmake/clang-10 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make
./ast-interpreter "测试程序"
```
实现了自动测试脚本test.sh, 可以直接在编译完成后运行./test.sh进行测试

## 功能拓展
目前已经完成/计划完成功能如下：
- [x] 全局变量
- [x] 函数调用
- [x] 分支及循环语句
- [x] 一维数组
- [x] 高维数组
- [ ] 一维指针
- [ ] 动态分配内存
- [ ] 高维指针