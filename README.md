# mini-json

一个从零实现的 JSON 解析器与生成器，C++17，header-only（全部实现只在 `json.hpp` 一个文件里）。

写它的目的不是造一个比 RapidJSON 更快的轮子，而是把**编译原理（递归下降）、C++ 内存管理（值语义与深拷贝）、错误处理（行列号定位）**这三件事真正动手做一遍，并且每一步都有可验证的测试。

---

## 特性

**支持**

- 完整 JSON 类型：`null` / `bool` / `number` / `string` / `array` / `object`
- 严格按 RFC 8259 文法校验数字，`01`、`1.`、`.5` 都会被拒绝
- 字符串转义：`\"` `\\` `\/` `\b` `\f` `\n` `\r` `\t`
- `\uXXXX`，含 UTF-16 代理对（例如 😀 = `\ud83d\ude00`）
- 解析错误报告到**第几行第几列**
- 嵌套深度保护（默认 64 层），恶意输入不会爆栈
- 序列化：`dump()` 紧凑输出，`dump(2)` 缩进美化
- 数字输出使用最短往返表示，`3.14` 不会变成 `3.1400000000000001`
- **21 个 GoogleTest 单元测试**，覆盖解析、序列化、错误定位与值语义

**不支持（有意为之，保持教学聚焦）**

- UTF-16 / UTF-32 编码的输入
- 区分整数与浮点数（内部统一用 `double`）
- JSON Pointer、JSON Schema、注释、尾随逗号等非标准扩展
- 原地（in-situ）解析、SIMD 优化等性能手段

---

## 编译与运行

依赖：支持 C++17 的编译器（GCC / Clang / MSVC），CMake 可选。

```bash
# 方式一：直接编译示例（json.hpp 是纯头文件，零依赖）
g++ -std=c++17 -O2 -o json_demo main.cpp
./json_demo

# 方式二：CMake（同时构建示例与单元测试）
cmake -S . -B build
cmake --build build
```

`main.cpp` 是一套零依赖的手动验证，全部通过时会输出 `[OK] step 10 passed.`。正式单元测试见下面的「测试」章节。

---

## 用法

```cpp
#include "json.hpp"

// 解析
mini_json::JsonValue v = mini_json::parse(R"({"name": "mini-json", "stars": 42})");

v.isObject();                          // true
v.asObject().at("name").asString();    // "mini-json"
v.asObject().at("stars").asNumber();   // 42

// 序列化
v.dump();    // {"name":"mini-json","stars":42}
v.dump(2);   // 缩进 2 空格的美化输出
```

错误处理：

```cpp
try {
    mini_json::parse("{\"a\": 1,}");
} catch (const mini_json::ParseException& e) {
    e.what();   // 错误原因
    e.line();   // 行号（从 1 开始）
    e.col();    // 列号（从 1 开始）
}
```

---

## 测试

项目里有**两套**测试，用途不同：

| 测试 | 位置 | 依赖 | 用途 |
|---|---|---|---|
| 手动验证 | `main.cpp` | 无 | 不装任何第三方库也能跑，适合快速自检与演示 |
| 单元测试 | `tests/test_json.cpp` | GoogleTest | 21 个用例、6 个测试组，适合回归与持续集成 |

单元测试分组：`ScalarParse`（标量）、`ContainerParse`（数组/对象/嵌套）、`StringParse`（转义与 Unicode）、`ErrorReport`（行列号与深度保护）、`Dump`（序列化与往返）、`ValueSemantics`（深拷贝、自赋值、类型错误）。

```bash
cmake -S . -B build && cmake --build build

./build/json_tests              # 看到 21 个用例逐条 PASSED
# 或者
cd build && ctest --output-on-failure
```

安装 GoogleTest（MSYS2 UCRT64）：

```bash
pacman -S mingw-w64-ucrt-x86_64-gtest
```

---

## 设计要点

| 主题 | 做法 | 为什么 |
|---|---|---|
| 解析策略 | 递归下降，`parse_value()` 按首字符分派 | 与 JSON 文法一一对应，代码即文法，易读易调试 |
| 值表示 | `enum Type` + 各类型成员；array / object 用 `std::unique_ptr` 间接持有 | 避免 `JsonValue` 因自引用导致对象体积爆炸 |
| 内存管理 | 拷贝时深拷贝，赋值用 copy-and-swap | 保证值语义：两个 `JsonValue` 不共享底层数据，且异常安全 |
| 错误定位 | `fail()` 扫描到出错偏移，换算成行列 | 报错对人友好，而不是只丢一个偏移量 |
| 递归安全 | `DepthGuard` 用 RAII 计数，超过 64 层主动报错 | 防止 `[[[[...]]]]` 这类输入导致栈溢出崩溃 |
| 数字输出 | 从 15 位精度起试，能原样转回同一个 `double` 就用它 | 最短往返，避免输出 `3.1400000000000001` |
| Unicode | `\uXXXX` 解出码点，代理对合成后再编码为 UTF-8 | 正确处理 emoji 等 BMP 之外的字符 |

---

## 项目结构

```
mini-json/
├── json.hpp              # 全部实现（header-only）
├── main.cpp              # 零依赖手动验证 + 用法示例
├── tests/
│   └── test_json.cpp     # GoogleTest 单元测试（21 个用例）
├── CMakeLists.txt
└── README.md
```

---

## 开发过程

这个项目按 10 个可验证的小步骤推进，每一步都有对应测试：

1. `null` / `true` / `false`
2. 数字（严格文法校验）
3. 字符串
4. 数组
5. 对象
6. 六种类型全支持
7. 转义序列
8. `\uXXXX` 与代理对
9. 行列号错误定位 + 深度保护
10. `dump()` 序列化 + 往返测试

每一步都能独立编译通过并打印 `[OK] step N passed.`，一旦出错可以立刻定位到是哪一步引入的问题。

---

## 已知限制

- 内部数字统一为 `double`，解析 `9223372036854775807` 这类大整数会损失精度
- 对象使用 `std::map`，查找为 O(log n)，且**不保留原始键顺序**（按字典序排列）
- 未做性能优化，也尚未与 RapidJSON 做基准对比
- 没有 `asBool()` 取值接口，布尔值目前只能用 `dump()` 或 `type()` 判断

---

## 下一步

- [x] 接入 GoogleTest，把现有测试改造成正式单测（21 个用例）
- [ ] 补 `asBool()` 等取值接口
- [ ] 增加 int64 / uint64 类型分支
- [ ] 做 benchmark，与 RapidJSON 对比并公开数据
- [ ] 补充 UTF-16 输入支持
