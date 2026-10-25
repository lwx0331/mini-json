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

// 期望解析出一个字符串，且内容等于 want
static void expect_string(const std::string& text, const std::string& want) {
    try {
        JsonValue v = mini_json::parse(text);
        check(v.isString() && v.asString() == want,
              "parse(" + text + ") -> \"" + v.asString() + "\"");
    } catch (const ParseException& e) {
        std::cout << "[FAIL] parse(" << text << ") threw: " << e.what() << "\n";
        ++failures;
    }
}

// 期望解析失败并抛异常
static void expect_error(const std::string& text) {
    try {
        mini_json::parse(text);
        std::cout << "[FAIL] expected error for " << text << " but none thrown\n";
        ++failures;
    } catch (const ParseException& e) {
        std::cout << "[ok] error for " << text << ": " << e.what()
                  << " (pos " << e.pos() << ")\n";
    }
}

int main() {
    // ---- 正常用例 ----
    expect_string("\"abc\"", "abc");
    expect_string("\"\"", "");                      // 空字符串
    expect_string("\"hello world\"", "hello world"); // 内部允许空格
    expect_string("  \"  padded  \"  ", "  padded  "); // 前后空白被跳过，内部保留
    expect_string("\"123\"", "123");                // 数字形态的字符串
    expect_string("\"中文\"", "中文");               // UTF-8 原样透传

    // ---- 上一步的字面量仍然要能解析（回归测试）----
    check(mini_json::parse("null").isNull(), "null still works");
    check(mini_json::parse("true").isBool(), "true still works");
    check(mini_json::parse("false").isBool(), "false still works");

    // ---- 异常用例 ----
    expect_error("\"abc");        // 没有结束引号
    expect_error("abc");          // 根本没有引号
    expect_error("\"a\"\"b\"");   // 尾部多余内容
    expect_error("\"a\nb\"");     // 字符串内不能出现裸换行（控制字符）

    if (failures == 0) {
        std::cout << "[OK] step 3 passed.\n";
        return 0;
    }
    std::cout << failures << " failure(s).\n";
    return 1;
}