#include <iostream>
#include "json.hpp"

using mini_json::JsonValue;

static void show(const char* label, const JsonValue& v) {
    std::cout << label << " -> type: " << v.typeName() << "\n";
}

int main() {
    JsonValue v_null;
    JsonValue v_bool(true);
    JsonValue v_num(3.14);
    JsonValue v_str(std::string("hello"));

    mini_json::Array arr;
    arr.push_back(JsonValue(1.0));
    arr.push_back(JsonValue(2.0));
    JsonValue v_array(arr);

    mini_json::Object obj;
    obj["name"] = JsonValue(std::string("mini-json"));
    obj["star"] = JsonValue(0.0);
    JsonValue v_object(obj);

    show("null", v_null);
    show("true", v_bool);
    show("3.14", v_num);
    show("\"hello\"", v_str);
    show("[1, 2]", v_array);
    show("{\"name\": ...}", v_object);

    if (!v_null.isNull()) std::cout << "[FAIL] isNull\n";
    if (!v_bool.isBool()) std::cout << "[FAIL] isBool\n";
    if (!v_num.isNumber()) std::cout << "[FAIL] isNumber\n";
    if (!v_str.isString()) std::cout << "[FAIL] isString\n";
    if (!v_array.isArray()) std::cout << "[FAIL] isArray\n";
    if (!v_object.isObject()) std::cout << "[FAIL] isObject\n";

    std::cout << "[OK] step 1 passed.\n";
    return 0;
}
