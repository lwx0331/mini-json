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
    // ---- 空对象 ----
    {
        JsonValue v = mini_json::parse("{}");
        check(v.isObject() && v.asObject().empty(), "{} -> empty object");
    }

    // ---- 单个键值对 ----
    {
        JsonValue v = mini_json::parse("{\"a\": 1}");
        bool ok = v.isObject() && v.asObject().size() == 1;
        if (ok)
            ok = v.asObject().at("a").asNumber() == 1;
        check(ok, "{\"a\": 1} -> a == 1");
    }

    // ---- 空白容错 ----
    {
        JsonValue v = mini_json::parse("  { \"a\" : 1 , \"b\" : 2 }  ");
        bool ok = v.isObject() && v.asObject().size() == 2;
        if (ok)
            ok = v.asObject().at("b").asNumber() == 2;
        check(ok, "spaced object -> 2 keys");
    }

    // ---- 混合类型的值 ----
    {
        JsonValue v =
            mini_json::parse("{\"n\": null, \"b\": true, \"s\": \"x\", \"f\": 1.5}");
        bool ok = v.isObject() && v.asObject().size() == 4;
        if (ok)
        {
            const auto &o = v.asObject();
            ok = o.at("n").isNull() && o.at("b").isBool() &&
                 o.at("s").isString() && o.at("f").isNumber();
        }
        check(ok, "object with mixed value types");
    }

    // ---- 嵌套：对象里套对象、套数组 ----
    {
        JsonValue v = mini_json::parse("{\"u\": {\"id\": 7}, \"tags\": [1, 2]}");
        bool ok = v.isObject() && v.asObject().size() == 2;
        if (ok)
        {
            const auto &o = v.asObject();
            ok = o.at("u").asObject().at("id").asNumber() == 7 &&
                 o.at("tags").asArray().size() == 2;
        }
        check(ok, "nested object + array");
    }

    // ---- 回归 ----
    check(mini_json::parse("null").isNull(), "null still works");
    check(mini_json::parse("\"abc\"").asString() == "abc", "string still works");
    check(std::fabs(mini_json::parse("-1.5e2").asNumber() + 150) < 1e-9,
          "number still works");
    check(mini_json::parse("[1, 2]").asArray().size() == 2, "array still works");

    // ---- 异常用例 ----
    expect_error("{");           // 未闭合
    expect_error("}");           // 孤立右括号
    expect_error("{\"a\"}");     // 缺冒号
    expect_error("{\"a\":}");    // 缺值
    expect_error("{a: 1}");      // 键不是字符串
    expect_error("{'a': 1}");    // 单引号非法
    expect_error("{\"a\": 1,}"); // 尾随逗号
    expect_error("{} {}");       // 尾部多余内容

    if (failures == 0)
    {
        std::cout << "[OK] step 6 passed.\n";
        return 0;
    }
    std::cout << failures << " failure(s).\n";
    return 1;
}