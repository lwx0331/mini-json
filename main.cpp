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

// 紧凑序列化后应等于 want
static void expect_dump(const std::string& text, const std::string& want) {
    try {
        JsonValue v = mini_json::parse(text);
        check(v.dump() == want, "dump(" + text + ") -> " + v.dump());
    } catch (const ParseException& e) {
        std::cout << "[FAIL] parse(" << text << ") threw: " << e.what() << "\n";
        ++failures;
    }
}

// 往返测试：parse -> dump -> parse -> dump，结果必须稳定
static void expect_roundtrip(const std::string& text) {
    try {
        const std::string once = mini_json::parse(text).dump();
        const std::string twice = mini_json::parse(once).dump();
        check(once == twice, "roundtrip stable: " + text);
    } catch (const ParseException& e) {
        std::cout << "[FAIL] roundtrip " << text << " threw: " << e.what() << "\n";
        ++failures;
    }
}

int main() {
    // ---- 标量序列化 ----
    expect_dump("null", "null");
    expect_dump("true", "true");
    expect_dump("false", "false");
    expect_dump("42", "42");
    expect_dump("-17", "-17");
    expect_dump("3.14", "3.14");
    expect_dump("\"abc\"", "\"abc\"");

    // ---- 容器序列化（紧凑）----
    expect_dump("[]", "[]");
    expect_dump("{}", "{}");
    expect_dump("[1, 2, 3]", "[1,2,3]");
    expect_dump("{\"a\": 1}", "{\"a\":1}");
    expect_dump("{\"a\": [1, {\"b\": null}]}", "{\"a\":[1,{\"b\":null}]}");

    // ---- 转义在输出端要还原 ----
    expect_dump("\"a\\nb\"", "\"a\\nb\"");        // 换行符输出成 \n
    expect_dump("\"say \\\"hi\\\"\"", "\"say \\\"hi\\\"\"");
    expect_dump("\"a\\\\b\"", "\"a\\\\b\"");

    // ---- Unicode：\uXXXX 转成 UTF-8 原文输出 ----
    expect_dump("\"\\u4e2d\\u6587\"", "\"\xE4\xB8\xAD\xE6\x96\x87\"");   // 中文
    expect_dump("\"\\ud83d\\ude00\"", "\"\xF0\x9F\x98\x80\"");           // 😀

    // ---- 往返稳定性 ----
    expect_roundtrip("{\"name\": \"mini-json\", \"tags\": [1, 2, 3], \"ok\": true}");
    expect_roundtrip("[1, 2.5, \"x\", null, false, {\"a\": {\"b\": []}}]");
    expect_roundtrip("\"\\ud83d\\ude00\\u4e2d\\u6587\"");
    expect_roundtrip("3.14159265358979");

    // ---- 美化输出（缩进 2 空格）----
    {
        JsonValue v = mini_json::parse("{\"a\": 1, \"b\": [2, 3]}");
        const std::string pretty = v.dump(2);
        std::cout << "--- pretty ---\n" << pretty << "\n--------------\n";
        check(pretty.find('\n') != std::string::npos, "dump(2) 有换行缩进");
        check(mini_json::parse(pretty).dump() == v.dump(),
              "美化后的文本仍能正确解析回来");
    }

    // ---- 回归：解析端不能被弄坏 ----
    check(mini_json::parse("null").isNull(), "null still works");
    check(mini_json::parse("\"abc\"").asString() == "abc", "string still works");
    check(std::fabs(mini_json::parse("-1.5e2").asNumber() + 150) < 1e-9,
          "number still works");
    check(mini_json::parse("\"\\ud83d\\ude00\"").asString() == "\xF0\x9F\x98\x80",
          "unicode escape still works");

    if (failures == 0) {
        std::cout << "[OK] step 10 passed.\n";
        return 0;
    }
    std::cout << failures << " failure(s).\n";
    return 1;
}