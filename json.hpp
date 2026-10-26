#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace mini_json
{

    class JsonValue;

    // 复合类型的容器：数组 = 值列表，对象 = 键值映射
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;

    // JSON 中的一个值。六种类型：null / bool / number / string / array / object
    class JsonValue
    {
    public:
        enum class Type
        {
            Null,
            Bool,
            Number,
            String,
            Array,
            Object
        };

        // ---------- 6 个构造函数 ----------
        JsonValue() : type_(Type::Null) {}

        JsonValue(bool b) : type_(Type::Bool), bool_(b) {}

        JsonValue(double d) : type_(Type::Number), num_(d) {}

        JsonValue(const std::string &s) : type_(Type::String), str_(s) {}

        JsonValue(const char *s) : type_(Type::String), str_(s) {}

        JsonValue(const Array &a)
            : type_(Type::Array), arr_(std::make_unique<Array>(a)) {}

        JsonValue(const Object &o)
            : type_(Type::Object), obj_(std::make_unique<Object>(o)) {}

        // ---------- 拷贝：复合类型必须深拷贝，否则两个值会共享同一份数据 ----------
        JsonValue(const JsonValue &other)
            : type_(other.type_),
              bool_(other.bool_),
              num_(other.num_),
              str_(other.str_),
              arr_(other.arr_ ? std::make_unique<Array>(*other.arr_) : nullptr),
              obj_(other.obj_ ? std::make_unique<Object>(*other.obj_) : nullptr) {}

        JsonValue &operator=(const JsonValue &other)
        {
            if (this != &other)
            {
                JsonValue tmp(other); // copy-and-swap：先造副本，再交换，异常安全
                swap(tmp);
            }
            return *this;
        }

        // ---------- 类型查询 ----------
        Type type() const { return type_; }

        bool isNull() const { return type_ == Type::Null; }
        bool isBool() const { return type_ == Type::Bool; }
        bool isNumber() const { return type_ == Type::Number; }
        bool isString() const { return type_ == Type::String; }
        bool isArray() const { return type_ == Type::Array; }
        bool isObject() const { return type_ == Type::Object; }

        void swap(JsonValue &other) noexcept
        {
            using std::swap;
            swap(type_, other.type_);
            swap(bool_, other.bool_);
            swap(num_, other.num_);
            swap(str_, other.str_);
            swap(arr_, other.arr_);
            swap(obj_, other.obj_);
        }

        const char *typeName() const
        {
            switch (type_)
            {
            case Type::Null:
                return "null";
            case Type::Bool:
                return "bool";
            case Type::Number:
                return "number";
            case Type::String:
                return "string";
            case Type::Array:
                return "array";
            case Type::Object:
                return "object";
            }
            return "unknown";
        }

        // 取值：类型不匹配时抛异常，避免静默返回错误数据
        double asNumber() const
        {
            if (!isNumber())
                throw std::runtime_error(" value is not a number ");
            return num_;
        }
        const Array &asArray() const
        {
            if (!isArray())
                throw std::runtime_error("value is not an array");
            return *arr_;
        }
        const std::string &asString() const
        {
            if (!isString())
                throw std::runtime_error("value is not a string");
            return str_;
        }

    private:
        Type type_ = Type::Null;
        bool bool_ = false;
        double num_ = 0;
        std::string str_;
        std::unique_ptr<Array> arr_;
        std::unique_ptr<Object> obj_;
    };

    // ---------- 解析异常：携带出错位置 ----------
    class ParseException : public std::runtime_error
    {
    public:
        ParseException(const std::string &msg, std::size_t pos)
            : std::runtime_error(msg), pos_(pos) {}

        std::size_t pos() const { return pos_; }

    private:
        std::size_t pos_;
    };

    // ---------- 递归下降解析器（第 2 步：只认 null / true / false） ----------
    class Parser
    {
    public:
        explicit Parser(const std::string &src) : src_(src) {}

        JsonValue parse()
        {
            skip_ws();
            JsonValue v = parse_value();
            skip_ws();
            if (pos_ != src_.size())
                fail("unexpected trailing content");
            return v;
        }

    private:
        const std::string &src_;
        std::size_t pos_ = 0;

        char peek() const { return pos_ < src_.size() ? src_[pos_] : '\0'; }

        // 跳过 JSON 允许的四种空白符
        void skip_ws()
        {
            while (pos_ < src_.size())
            {
                const char c = src_[pos_];
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                {
                    ++pos_;
                }
                else
                {
                    break;
                }
            }
        }

        // 匹配一个字面量（null / true / false），成功则前进并返回对应值
        JsonValue parse_literal(const char *lit, JsonValue result)
        {
            const std::size_t n = std::strlen(lit);
            if (src_.compare(pos_, n, lit) != 0)
            {
                fail(std::string("invalid literal: expected ") + lit);
            }
            pos_ += n;
            return result;
        }

        static bool is_digit(char c) { return c >= '0' && c <= '9'; }

        // 严格按照JSON文法扫描数字，再交给strtod转成double
        // 文法：-?(0 | [1-09][0-9]*)(.[0-9]+)?([eE][+-]?[0-9]+)?
        JsonValue parse_number()
        {

            const std::size_t start = pos_;

            if (peek() == '-')
                ++pos_;

            // 整数部分:要么单个0,要么1-9开头后跟任意数字
            if (peek() == '0')
            {
                ++pos_;
                if (is_digit(peek()))
                    fail("invalid number: leading zero");
            }
            else if (is_digit(peek()))
            {
                while (is_digit(peek()))
                    ++pos_;
            }
            else
            {
                fail("invalid number");
            }

            // 小数部分
            if (peek() == '.')
            {
                ++pos_;
                if (!is_digit(peek()))
                    fail("invalid number: expected digit after decimal point");
                while (is_digit(peek()))
                    ++pos_;
            }

            // 指数部分
            if (peek() == 'e' || peek() == 'E')
            {
                ++pos_;
                if (peek() == '+' || peek() == '-')
                    ++pos_;
                if (!is_digit(peek()))
                    fail("invalid number: expected digit in exponent");
                while (is_digit(peek()))
                    ++pos_;
            }

            const std::string lexeme = src_.substr(start, pos_ - start);
            return JsonValue(std::strtod(lexeme.c_str(), nullptr));
        }
        // 读取一个数组：[ value (, value)* ]
        JsonValue parse_array()
        {
            if (peek() != '[')
                fail("expected array");
            ++pos_; // 跳过 '['

            Array arr;
            skip_ws();
            if (peek() == ']')
            { // 空数组 []
                ++pos_;
                return JsonValue(arr);
            }

            while (true)
            {
                skip_ws();
                arr.push_back(parse_value()); // ← 关键：递归调用，元素可以是任意 JSON 值
                skip_ws();

                if (peek() == ',')
                { // 还有下一个元素
                    ++pos_;
                    continue;
                }
                if (peek() == ']')
                { // 结束
                    ++pos_;
                    break;
                }
                fail("expected ',' or ']' in array");
            }
            return JsonValue(arr);
        }
        // 读取一个字符串（第 3 步：暂不处理转义序列）
        std::string parse_string()
        {
            if (peek() != '"')
                fail("expected string");
            ++pos_; // 跳过开头的引号

            std::string out;
            while (true)
            {
                if (pos_ >= src_.size())
                    fail("unterminated string");
                const char c = src_[pos_];
                if (c == '"')
                { // 遇到结束引号
                    ++pos_;
                    break;
                }
                // JSON 规定字符串中不能直接出现 < 0x20 的控制字符
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    fail("invalid control character in string");
                }
                out += c;
                ++pos_;
            }
            return out;
        }

        // 看首字符决定解析哪种值 —— 这就是「递归下降」的分派入口
        JsonValue parse_value()
        {
            switch (peek())
            {
            case '"':
                return JsonValue(parse_string());
            case 'n':
                return parse_literal("null", JsonValue());
            case 't':
                return parse_literal("true", JsonValue(true));
            case 'f':
                return parse_literal("false", JsonValue(false));
            case '[':
                return parse_array();  
            default:
                if (is_digit(peek()) || peek() == '-')
                    return parse_number();
                fail("unsupported value (step 5: null/true/false/string/number/array)");
            }
            return JsonValue(); // 不可达，只为消除编译器警告
        }

        [[noreturn]] void fail(const std::string &msg)
        {
            throw ParseException(msg, pos_);
        }
    };

    // 便捷入口
    inline JsonValue parse(const std::string &text) { return Parser(text).parse(); }

} // namespace mini_json
