根据提供的测试代码，`TestVector()` 函数覆盖了 `farmat::vector` 的以下功能点：

## 一、构造与析构
- 默认构造
- 带大小参数的构造
- 带大小和初始值的构造
- 拷贝构造
- 移动构造（C++11）
- 初始化列表构造（C++11）
- 带自定义分配器的构造
- 析构（隐式通过 RAII 验证）

## 二、赋值操作
- 拷贝赋值
- 移动赋值
- 初始化列表赋值

## 三、迭代器与访问
- `begin()` / `end()`（普通、const）
- `rbegin()` / `rend()`
- `cbegin()` / `cend()` / `crbegin()` / `crend()`（C++11）
- `operator[]`
- `at()`（含异常测试）
- `front()` / `back()`
- `data()`

## 四、容量操作
- `empty()`
- `size()`
- `capacity()`
- `reserve()`
- `shrink_to_fit()`
- `resize()`
- `set_capacity()`
- `reset_lose_memory()`

## 五、修改器（Modifiers）
- `push_back`（左值、右值、未初始化版本）
- `pop_back`
- `emplace_back` / `emplace`
- `insert`（单元素、重复元素、区间、初始化列表）
- `erase`（单位置、区间）
- `erase_unsorted`（位置、反向迭代器）
- `erase_first` / `erase_last` / `erase_first_unsorted` / `erase_last_unsorted`
- `clear`
- `swap`

## 八、元素类型要求与异常安全
- 含 `const` 成员的类型（`ItemWithConst`、`StructWithConstInt`）
- 含 `const&` 成员的类型
- 不可拷贝但可移动的类型（`testmovable`、`unique_ptr`）
- 用户重载 `operator&` 的类型（`HasAddressOfOperator`）
- 自移动赋值（`TestMoveAssignToSelf`）
- 插入/移动时自引用（从容器内移动/复制元素）


## 十一、比较运算符
- `==`、`!=`、`<`、`<=`、`>`、`>=`


## 十二、回归测试与特定问题
- 空向量 `data()` 返回 `NULL`
- 自赋值 `operator=` 内存泄漏
- 不必要地使用 `operator<` 的规避（自定义迭代器无 `<` 比较）
- VS 特定警告禁用与编译问题修复（如 `ScenarioRefEntry` 含 const 成员）

## 十三、其他
- `validate()` 与 `validate_iterator()` 调试支持
- C++11 范围 for 循环
- `emplace` / `push_back` 使用容器内自身元素
---