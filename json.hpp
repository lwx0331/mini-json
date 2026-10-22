#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mini_json {

class JsonValue;

using Array  = std::vector<JsonValue>;
using Object = std::map<std::string, JsonValue>;

class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    JsonValue() : type_(Type::Null) {}
    JsonValue(bool b) : type_(Type::Bool), bool_(b) {}
    JsonValue(double d) : type_(Type::Number), num_(d) {}
    JsonValue(const std::string& s) : type_(Type::String), str_(s) {}
    JsonValue(const char* s) : type_(Type::String), str_(s) {}
    JsonValue(const Array& a) : type_(Type::Array), arr_(std::make_unique<Array>(a)) {}
    JsonValue(const Object& o) : type_(Type::Object), obj_(std::make_unique<Object>(o)) {}

    JsonValue(const JsonValue& other)
        : type_(other.type_), bool_(other.bool_), num_(other.num_), str_(other.str_),
          arr_(other.arr_ ? std::make_unique<Array>(*other.arr_) : nullptr),
          obj_(other.obj_ ? std::make_unique<Object>(*other.obj_) : nullptr) {}

    JsonValue& operator=(const JsonValue& other) {
        if (this != &other) {
            JsonValue tmp(other);
            swap(tmp);
        }
        return *this;
    }

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isBool() const { return type_ == Type::Bool; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isString() const { return type_ == Type::String; }
    bool isArray() const { return type_ == Type::Array; }
    bool isObject() const { return type_ == Type::Object; }

    void swap(JsonValue& other) noexcept {
        using std::swap;
        swap(type_, other.type_);
        swap(bool_, other.bool_);
        swap(num_, other.num_);
        swap(str_, other.str_);
        swap(arr_, other.arr_);
        swap(obj_, other.obj_);
    }

    const char* typeName() const {
        switch (type_) {
            case Type::Null: return "null";
            case Type::Bool: return "bool";
            case Type::Number: return "number";
            case Type::String: return "string";
            case Type::Array: return "array";
            case Type::Object: return "object";
        }
        return "unknown";
    }

private:
    Type type_ = Type::Null;
    bool bool_ = false;
    double num_ = 0;
    std::string str_;
    std::unique_ptr<Array> arr_;
    std::unique_ptr<Object> obj_;
};

}  // namespace mini_json
