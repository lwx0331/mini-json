#include <cmath>
#include <iostream>
#include <string>

#include "json.hpp"

using mini_json::JsonValue;
using mini_json::ParseException;

static int failures = 0;

static void check(bool cond, const std::string &what)
{
    if (!cond)
    {
        std::cout << "[FAIL] " << what << "\n";
        ++failures;
    }
    else
    {
        std::cout << "[ok] " << what << "\n";
    }
}

// 期望解析出一个字符串，且内容逐字节等于 want
static void expect_string(const std::string &text, const std::string &want)
{
    try
    {
        JsonValue v = mini_json::parse(text);
        check(v.isString() && v.asString() == want, "parse(" + text + ")");
    }
    catch (const ParseException &e)
    {
        std::cout << "[FAIL] parse(" << text << ") threw: " << e.what() << "\n";
        ++failures;
    }
}

static void expect_error(const std::string &text)
{
    try
    {
        mini_json::parse(text);
        std::cout << "[FAIL] expected error for " << text << " but none thrown\n";
        ++failures;
    }
    catch (const ParseException &e)
    {
        std::cout << "[ok] error for " << text << ": " << e.what()
                  << " (pos " << e.pos() << ")\n";
    }
}

int main()
{
    // ---- 九种合法转义 ----
    expect_string("\"\\\"\"", "\""); // \"
    expect_string("\"\\\\\"", "\\"); // 反斜杠自身
    expect_string("\"\\/\"", "/");   // \/
    expect_string("\"\\b\"", "\b");  // 退格
    expect_string("\"\\f\"", "\f");  // 换页
    expect_string("\"\\n\"", "\n");  // 换行
    expect_string("\"\\r\"", "\r");  // 回车
    expect_string("\"\\t\"", "\t");  // 制表符

    // ---- 转义混在普通文本里 ----
    expect_string("\"a\\nb\"", "a\nb");
    expect_string("\"line1\\nline2\\tend\"", "line1\nline2\tend");
    expect_string("\"say \\\"hi\\\"\"", "say \"hi\"");

    // ---- 反斜杠后接普通字符要报错 ----
    expect_error("\"\\x\"");   // 非法转义
    expect_error("\"\\a\"");   // 非法转义
    expect_error("\"abc\\\""); // 结尾的反斜杠把引号吃掉了，字符串未闭合

    // ---- 回归：前面几步的功能不能被弄坏 ----
    check(mini_json::parse("null").isNull(), "null still works");
    check(mini_json::parse("\"abc\"").asString() == "abc", "string still works");
    check(std::fabs(mini_json::parse("-1.5e2").asNumber() + 150) < 1e-9,
          "number still works");
    check(mini_json::parse("[1, 2]").asArray().size() == 2, "array still works");
    check(mini_json::parse("{\"a\": 1}").asObject().at("a").asNumber() == 1,
          "object still works");

    if (failures == 0)
    {
        std::cout << "[OK] step 7 passed.\n";
        return 0;
    }
    std::cout << failures << " failure(s).\n";
    return 1;
}