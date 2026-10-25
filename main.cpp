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

// 期望解析出一个数字，且数值等于 want
static void expect_number(const std::string& text, double want) {
    try {
        JsonValue v = mini_json::parse(text);
        const bool ok = v.isNumber() && std::fabs(v.asNumber() - want) < 1e-12;
        check(ok, "parse(" + text + ") = " + std::to_string(v.asNumber()));
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
    // ---- 正常用例 ----
    expect_number("0", 0);
    expect_number("-0", 0);          // JSON 允许负零
    expect_number("42", 42);
    expect_number("-17", -17);
    expect_number("3.14", 3.14);
    expect_number("-1.5e2", -150);   // 科学计数法
    expect_number("1E+3", 1000);
    expect_number("1e-2", 0.01);
    expect_number("2.5e0", 2.5);

    // ---- 回归：前面几步的功能不能被弄坏 ----
    check(mini_json::parse("null").isNull(), "null still works");
    check(mini_json::parse("\"abc\"").asString() == "abc", "string still works");

    // ---- 异常用例：JSON 数字文法比 C 的 strtod 严格得多 ----
    expect_error("01");      // 前导零
    expect_error("00");
    expect_error("-01");
    expect_error("1.");      // 小数点后必须至少有 1 位
    expect_error(".5");      // 必须以数字开头
    expect_error("+1");      // 不允许前导正号
    expect_error("1e");      // 指数部分缺数字
    expect_error("1e+");
    expect_error("-");       // 只有负号

    if (failures == 0) {
        std::cout << "[OK] step 4 passed.\n";
        return 0;
    }
    std::cout << failures << " failure(s).\n";
    return 1;
}