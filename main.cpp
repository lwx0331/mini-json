#include <cmath>
#include <iostream>
#include <string>

#include "json.hpp"

using mini_json::JsonValue;
using mini_json::ParseException;

static int failures = 0;

static void check(bool cond, const std::string& what) {
    if (!cond) {
        std::cout << "[FAIL] " << what << "\n";
        ++failures;
    } else {
        std::cout << "[ok] " << what << "\n";
    }
}

// 期望报错，且行号列号完全对得上
static void expect_error_at(const std::string& text, std::size_t line,
                            std::size_t col, const std::string& label) {
    try {
        mini_json::parse(text);
        std::cout << "[FAIL] " << label << ": no exception thrown\n";
        ++failures;
    } catch (const ParseException& e) {
        check(e.line() == line && e.col() == col,
              label + ": " + e.what() + " (line " + std::to_string(e.line()) +
                  ", col " + std::to_string(e.col()) + ")");
    }
}

static void expect_error(const std::string& text) {
    try {
        mini_json::parse(text);
        std::cout << "[FAIL] expected error for " << text << " but none thrown\n";
        ++failures;
    } catch (const ParseException& e) {
        std::cout << "[ok] error for " << text << ": " << e.what()
                  << " (line " << e.line() << ", col " << e.col() << ")\n";
    }
}

int main() {
    // ---- 单行错误：列号 ----
    expect_error_at("[1, 2", 1, 6, "数组未闭合");       // 错在最后那个字符之后
    expect_error_at("{\"a\" 1}", 1, 6, "对象缺冒号");
    expect_error_at("{\"a\": ,}", 1, 7, "对象缺值");

    // ---- 多行错误：行号 ----
    expect_error_at("{\n  \"a\": 1,\n  \"b\": ,\n}", 3, 8, "第 3 行的错误");
    expect_error_at("[\n  1,\n  2\n", 4, 1, "多行数组未闭合");

    // ---- 嵌套深度保护 ----
    {
        // 60 层：应当正常解析
        std::string ok60;
        for (int i = 0; i < 60; ++i) ok60 += "[";
        for (int i = 0; i < 60; ++i) ok60 += "]";
        bool parsed = false;
        try {
            mini_json::parse(ok60);
            parsed = true;
        } catch (const ParseException&) {
        }
        check(parsed, "60 层嵌套可以正常解析");

        // 500 层：必须被拦住，而不是栈溢出崩溃
        std::string deep;
        for (int i = 0; i < 500; ++i) deep += "[";
        for (int i = 0; i < 500; ++i) deep += "]";
        bool blocked = false;
        try {
            mini_json::parse(deep);
        } catch (const ParseException& e) {
            blocked = std::string(e.what()).find("depth") != std::string::npos;
        }
        check(blocked, "500 层嵌套被深度保护拦截（不会爆栈）");
    }

    // ---- 回归：前面所有步骤都不能被弄坏 ----
    check(mini_json::parse("null").isNull(), "null still works");
    check(mini_json::parse("\"abc\"").asString() == "abc", "string still works");
    check(std::fabs(mini_json::parse("-1.5e2").asNumber() + 150) < 1e-9,
          "number still works");
    check(mini_json::parse("[1, 2]").asArray().size() == 2, "array still works");
    check(mini_json::parse("{\"a\": 1}").asObject().at("a").asNumber() == 1,
          "object still works");
    check(mini_json::parse("\"a\\nb\"").asString() == "a\nb", "escape still works");
    check(mini_json::parse("\"\\ud83d\\ude00\"").asString() == "\xF0\x9F\x98\x80",
          "unicode escape still works");

    // ---- 其他非法输入仍然报错 ----
    expect_error("01");
    expect_error("\"\\x\"");
    expect_error("{a: 1}");

    if (failures == 0) {
        std::cout << "[OK] step 9 passed.\n";
        return 0;
    }
    std::cout << failures << " failure(s).\n";
    return 1;
}