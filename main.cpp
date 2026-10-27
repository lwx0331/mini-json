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

// 期望解析出一个字符串，且内容逐字节等于 want
static void expect_string(const std::string& text, const std::string& want) {
    try {
        JsonValue v = mini_json::parse(text);
        check(v.isString() && v.asString() == want, "parse(" + text + ")");
    } catch (const ParseException& e) {
        std::cout << "[FAIL] parse(" << text << ") threw: " << e.what() << "\n";
        ++failures;
    }
}

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
    // ---- BMP 基本区：\uXXXX ----
    expect_string("\"\\u0041\"", "A");              // 大写字母 A
    expect_string("\"\\u0042\\u0043\"", "BC");        // 连续两个
    expect_string("\"a\\u0041b\"", "aAb");           // 与普通字符混排

    // ---- 2 字节 UTF-8（U+0080 ~ U+07FF）----
    expect_string("\"\\u00e9\"", "\xC3\xA9");         // é
    expect_string("\"caf\\u00e9\"", "caf\xC3\xA9");  // café

    // ---- 3 字节 UTF-8（U+0800 ~ U+FFFF）—— 中文就在这里 ----
    expect_string("\"\\u4e2d\\u6587\"", "\xE4\xB8\xAD\xE6\x96\x87");  // 中文
    expect_string("\"\\u4f60\\u597d\"", "\xE4\xBD\xA0\xE5\xA5\xBD");  // 你好

    // ---- 4 字节 UTF-8：必须靠代理对 ----
    // U+1F600 = 😀，UTF-16 代理对为 D83D DE00
    expect_string("\"\\ud83d\\ude00\"", "\xF0\x9F\x98\x80");        // 😀
    expect_string("\"hi \\ud83d\\ude00\"", "hi \xF0\x9F\x98\x80");   // 混排

    // ---- 非法 Unicode 转义 ----
    expect_error("\"\\u00\"");       // 十六进制不足 4 位
    expect_error("\"\\uZZZZ\"");     // 非十六进制字符
    expect_error("\"\\ud83d\"");      // 孤立高代理项（后面没有低代理项）
    expect_error("\"\\ude00\"");      // 孤立低代理项

    // ---- 回归：前面几步的功能不能被弄坏 ----
    check(mini_json::parse("null").isNull(), "null still works");
    check(mini_json::parse("\"abc\"").asString() == "abc", "string still works");
    check(std::fabs(mini_json::parse("-1.5e2").asNumber() + 150) < 1e-9,
          "number still works");
    check(mini_json::parse("[1, 2]").asArray().size() == 2, "array still works");
    check(mini_json::parse("{\"a\": 1}").asObject().at("a").asNumber() == 1,
          "object still works");
    check(mini_json::parse("\"a\\nb\"").asString() == "a\nb", "escape still works");

    if (failures == 0) {
        std::cout << "[OK] step 8 passed.\n";
        return 0;
    }
    std::cout << failures << " failure(s).\n";
    return 1;
}
