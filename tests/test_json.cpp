#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "json.hpp"

using mini_json::JsonValue;
using mini_json::ParseException;

namespace {

// 断言这段文本一定解析失败
void expect_error(const std::string& text) {
    EXPECT_THROW(mini_json::parse(text), ParseException) << "input: " << text;
}

std::string repeat(const std::string& s, int n) {
    std::string out;
    for (int i = 0; i < n; ++i) out += s;
    return out;
}

}  // namespace

// ================= 步骤 1~3：标量 =================

TEST(ScalarParse, Literals) {
    EXPECT_TRUE(mini_json::parse("null").isNull());
    EXPECT_TRUE(mini_json::parse("true").isBool());
    EXPECT_TRUE(mini_json::parse("false").isBool());

    // 当前 API 没有 asBool()，用 dump() 区分 true / false
    EXPECT_EQ(mini_json::parse("true").dump(), "true");
    EXPECT_EQ(mini_json::parse("false").dump(), "false");
    EXPECT_EQ(mini_json::parse("null").dump(), "null");
}

TEST(ScalarParse, Numbers) {
    EXPECT_DOUBLE_EQ(mini_json::parse("42").asNumber(), 42.0);
    EXPECT_DOUBLE_EQ(mini_json::parse("-17").asNumber(), -17.0);
    EXPECT_DOUBLE_EQ(mini_json::parse("3.14").asNumber(), 3.14);
    EXPECT_DOUBLE_EQ(mini_json::parse("-1.5e2").asNumber(), -150.0);
    EXPECT_DOUBLE_EQ(mini_json::parse("1E3").asNumber(), 1000.0);
    EXPECT_DOUBLE_EQ(mini_json::parse("0").asNumber(), 0.0);
}

TEST(ScalarParse, RejectsInvalidNumbers) {
    expect_error("01");     // 前导零
    expect_error("1.");     // 小数点后缺数字
    expect_error(".5");     // 缺整数部分
    expect_error("1e");     // 指数缺数字
    expect_error("--1");    // 双负号
    expect_error("+1");     // JSON 不允许前导 +
}

TEST(ScalarParse, Strings) {
    EXPECT_EQ(mini_json::parse("\"abc\"").asString(), "abc");
    EXPECT_EQ(mini_json::parse("\"\"").asString(), "");
    EXPECT_EQ(mini_json::parse("\"中文\"").asString(), "中文");
}

// ================= 步骤 4~6：数组与对象 =================

TEST(ContainerParse, Arrays) {
    JsonValue v = mini_json::parse("[1, 2, 3]");
    ASSERT_TRUE(v.isArray());
    ASSERT_EQ(v.asArray().size(), 3u);
    EXPECT_DOUBLE_EQ(v.asArray()[0].asNumber(), 1.0);
    EXPECT_DOUBLE_EQ(v.asArray()[2].asNumber(), 3.0);
    EXPECT_TRUE(mini_json::parse("[]").asArray().empty());
}

TEST(ContainerParse, Objects) {
    JsonValue v = mini_json::parse("{\"a\": 1, \"b\": [2, 3]}");
    ASSERT_TRUE(v.isObject());
    EXPECT_DOUBLE_EQ(v.asObject().at("a").asNumber(), 1.0);
    EXPECT_TRUE(v.asObject().at("b").isArray());
    EXPECT_TRUE(mini_json::parse("{}").asObject().empty());
}

TEST(ContainerParse, NestedStructures) {
    JsonValue v = mini_json::parse("{\"a\": [1, {\"b\": null}], \"c\": {\"d\": []}}");
    ASSERT_TRUE(v.isObject());
    EXPECT_TRUE(v.asObject().at("c").asObject().at("d").asArray().empty());
    EXPECT_TRUE(v.asObject().at("a").asArray()[1].asObject().at("b").isNull());
}

TEST(ContainerParse, RejectsMalformedInput) {
    expect_error("[1, 2");        // 数组未闭合
    expect_error("{\"a\" 1}");    // 缺冒号
    expect_error("{\"a\": ,}");   // 缺值
    expect_error("[1,]");         // 尾随逗号
    expect_error("{a: 1}");       // 键不是字符串
    expect_error("{\"a\": 1");    // 对象未闭合
}

// ================= 步骤 7~8：转义与 Unicode =================

TEST(StringParse, EscapeSequences) {
    EXPECT_EQ(mini_json::parse("\"a\\nb\"").asString(), "a\nb");
    EXPECT_EQ(mini_json::parse("\"a\\tb\"").asString(), "a\tb");
    EXPECT_EQ(mini_json::parse("\"a\\\\b\"").asString(), "a\\b");
    EXPECT_EQ(mini_json::parse("\"a\\\"b\"").asString(), "a\"b");
    EXPECT_EQ(mini_json::parse("\"a\\/b\"").asString(), "a/b");
}

TEST(StringParse, UnicodeEscapes) {
    // \uXXXX 解出码点后编码为 UTF-8
    EXPECT_EQ(mini_json::parse("\"\\u4e2d\\u6587\"").asString(),
              "\xE4\xB8\xAD\xE6\x96\x87");
    // 代理对：😀
    EXPECT_EQ(mini_json::parse("\"\\ud83d\\ude00\"").asString(),
              "\xF0\x9F\x98\x80");
}

TEST(StringParse, RejectsBadEscapes) {
    expect_error("\"\\x\"");       // 非法转义字符
    expect_error("\"\\u12\"");     // 十六进制不足 4 位
    expect_error("\"\\ud83d\"");   // 孤立的高代理项
    expect_error("\"\\ude00\"");   // 孤立的低代理项
    expect_error("\"abc");         // 字符串未闭合
}

// ================= 步骤 9：错误定位与深度保护 =================

TEST(ErrorReport, ReportsLineAndColumn) {
    try {
        mini_json::parse("[1, 2");
        FAIL() << "应当抛出 ParseException";
    } catch (const ParseException& e) {
        EXPECT_EQ(e.line(), 1u);
        EXPECT_EQ(e.col(), 6u);
    }

    try {
        mini_json::parse("{\n  \"a\": 1,\n  \"b\": ,\n}");
        FAIL() << "应当抛出 ParseException";
    } catch (const ParseException& e) {
        EXPECT_EQ(e.line(), 3u);
        EXPECT_EQ(e.col(), 8u);
    }
}

TEST(ErrorReport, DepthLimitProtectsStack) {
    const std::string deep = repeat("[", 500) + repeat("]", 500);
    try {
        mini_json::parse(deep);
        FAIL() << "500 层嵌套应当被深度保护拦截";
    } catch (const ParseException& e) {
        EXPECT_NE(std::string(e.what()).find("depth"), std::string::npos);
    }

    // 64 层以内应当正常解析
    const std::string ok = repeat("[", 60) + repeat("]", 60);
    EXPECT_NO_THROW(mini_json::parse(ok));
}

// ================= 步骤 10：序列化 =================

TEST(Dump, CompactOutput) {
    EXPECT_EQ(mini_json::parse("null").dump(), "null");
    EXPECT_EQ(mini_json::parse("42").dump(), "42");
    EXPECT_EQ(mini_json::parse("-17").dump(), "-17");
    EXPECT_EQ(mini_json::parse("3.14").dump(), "3.14");  // 最短往返，不是 3.1400000000000001
    EXPECT_EQ(mini_json::parse("\"abc\"").dump(), "\"abc\"");
    EXPECT_EQ(mini_json::parse("[]").dump(), "[]");
    EXPECT_EQ(mini_json::parse("{}").dump(), "{}");
    EXPECT_EQ(mini_json::parse("[1, 2, 3]").dump(), "[1,2,3]");
    EXPECT_EQ(mini_json::parse("{\"a\": 1}").dump(), "{\"a\":1}");
    EXPECT_EQ(mini_json::parse("{\"a\": [1, {\"b\": null}]}").dump(),
              "{\"a\":[1,{\"b\":null}]}");
}

TEST(Dump, EscapesOnOutput) {
    EXPECT_EQ(mini_json::parse("\"a\\nb\"").dump(), "\"a\\nb\"");
    EXPECT_EQ(mini_json::parse("\"say \\\"hi\\\"\"").dump(), "\"say \\\"hi\\\"\"");
    EXPECT_EQ(mini_json::parse("\"a\\\\b\"").dump(), "\"a\\\\b\"");
    // \uXXXX 解出来的字符按 UTF-8 原文输出
    EXPECT_EQ(mini_json::parse("\"\\u4e2d\\u6587\"").dump(),
              "\"\xE4\xB8\xAD\xE6\x96\x87\"");
}

TEST(Dump, KeyOrderFollowsStdMap) {
    // 已知限制：std::map 按字典序排列，不保留原始键顺序
    EXPECT_EQ(mini_json::parse("{\"b\": 1, \"a\": 2}").dump(), "{\"a\":2,\"b\":1}");
}

TEST(Dump, PrettyPrintIsStillValidJson) {
    JsonValue v = mini_json::parse("{\"a\": 1, \"b\": [2, 3]}");
    const std::string pretty = v.dump(2);
    EXPECT_NE(pretty.find('\n'), std::string::npos);
    EXPECT_EQ(mini_json::parse(pretty).dump(), v.dump());
}

TEST(Dump, RoundTripIsStable) {
    const char* cases[] = {
        "{\"name\": \"mini-json\", \"tags\": [1, 2, 3], \"ok\": true}",
        "[1, 2.5, \"x\", null, false, {\"a\": {\"b\": []}}]",
        "\"\\ud83d\\ude00\\u4e2d\\u6587\"",
        "3.14159265358979",
    };
    for (const char* text : cases) {
        const std::string once = mini_json::parse(text).dump();
        const std::string twice = mini_json::parse(once).dump();
        EXPECT_EQ(once, twice) << "unstable roundtrip: " << text;
    }
}

// ================= 内存管理：值语义与深拷贝 =================

TEST(ValueSemantics, CopyAndAssignAreIndependent) {
    JsonValue a = mini_json::parse("[1, {\"k\": [2]}]");
    JsonValue b = a;   // 拷贝构造（深拷贝）
    JsonValue c;
    c = a;             // 拷贝赋值（copy-and-swap）

    EXPECT_EQ(a.dump(), b.dump());
    EXPECT_EQ(a.dump(), c.dump());

    // 三个对象各自持有独立数据，析构时不会 double free
    // （配合 -fsanitize=address 编译运行可验证）
}

TEST(ValueSemantics, SelfAssignmentIsSafe) {
    JsonValue a = mini_json::parse("{\"x\": 1}");
    a = a;             // 自赋值必须安全
    EXPECT_EQ(a.dump(), "{\"x\":1}");
}

TEST(ValueSemantics, TypeMismatchThrows) {
    JsonValue v = mini_json::parse("42");
    EXPECT_THROW(v.asString(), std::runtime_error);
    EXPECT_THROW(v.asArray(), std::runtime_error);
    EXPECT_THROW(v.asObject(), std::runtime_error);
}
