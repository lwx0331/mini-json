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
    // ---- 空数组 ----
    {
        JsonValue v = mini_json::parse("[]");
        check(v.isArray() && v.asArray().empty(), "[] -> empty array");
    }

    // ---- 基本元素 ----
    {
        JsonValue v = mini_json::parse("[1, 2, 3]");
        check(v.isArray() && v.asArray().size() == 3, "[1, 2, 3] -> size 3");
        if (v.isArray()) {
            check(v.asArray()[0].asNumber() == 1, "  [0] == 1");
            check(v.asArray()[1].asNumber() == 2, "  [1] == 2");
        }
    }

    // ---- 空白容错 ----
    {
        JsonValue v = mini_json::parse("  [ 1 , 2 ]  ");
        check(v.isArray() && v.asArray().size() == 2, "[ 1 , 2 ] with spaces -> size 2");
    }

    // ---- 混合类型元素 ----
    {
        JsonValue v = mini_json::parse("[null, true, \"hi\", 1.5]");
        bool ok = v.isArray() && v.asArray().size() == 4;
        if (ok) {
            ok = v.asArray()[0].isNull() && v.asArray()[1].isBool() &&
                 v.asArray()[2].isString() && v.asArray()[3].isNumber();
        }
        check(ok, "[null, true, \"hi\", 1.5] -> mixed types");
    }

    // ---- 嵌套数组（递归下降的真正威力）----
    {
        JsonValue v = mini_json::parse("[[1, 2], [3, [4, 5]]]");
        bool ok = v.isArray() && v.asArray().size() == 2;
        if (ok) {
            const auto& inner = v.asArray()[1].asArray();
            ok = v.asArray()[0].asArray().size() == 2 && inner.size() == 2 &&
                 inner[1].asArray().size() == 2 &&
                 inner[1].asArray()[1].asNumber() == 5;
        }
        check(ok, "[[1,2],[3,[4,5]]] -> nested arrays");
    }

    // ---- 回归 ----
    check(mini_json::parse("null").isNull(), "null still works");
    check(mini_json::parse("\"abc\"").asString() == "abc", "string still works");
    check(std::fabs(mini_json::parse("-1.5e2").asNumber() + 150) < 1e-9,
          "number still works");

    // ---- 异常用例 ----
    expect_error("[1, 2");      // 未闭合
    expect_error("[1,]");       // 尾随逗号
    expect_error("[,1]");       // 前导逗号
    expect_error("[1 2]");      // 缺逗号
    expect_error("[");          // 只有左括号
    expect_error("]");          // 孤立右括号

    if (failures == 0) {
        std::cout << "[OK] step 5 passed.\n";
        return 0;
    }
    std::cout << failures << " failure(s).\n";
    return 1;
}