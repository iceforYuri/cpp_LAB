#ifndef JSON_HPP
#define JSON_HPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <memory>
#include <algorithm>

namespace nlohmann {

// 简化版的JSON类，仅提供基本功能
class json {
public:
    enum class value_t {
        null,
        object,
        array,
        string,
        boolean,
        number_integer,
        number_float
    };

private:
    struct json_value {
        virtual ~json_value() = default;
        virtual value_t type() const = 0;
        virtual std::shared_ptr<json_value> clone() const = 0;
    };

    struct null_t : json_value {
        value_t type() const override { return value_t::null; }
        std::shared_ptr<json_value> clone() const override { return std::make_shared<null_t>(); }
    };

    struct boolean_t : json_value {
        bool value;
        explicit boolean_t(bool v) : value(v) {}
        value_t type() const override { return value_t::boolean; }
        std::shared_ptr<json_value> clone() const override { return std::make_shared<boolean_t>(value); }
    };

    struct string_t : json_value {
        std::string value;
        explicit string_t(std::string v) : value(std::move(v)) {}
        value_t type() const override { return value_t::string; }
        std::shared_ptr<json_value> clone() const override { return std::make_shared<string_t>(value); }
    };

    struct number_integer_t : json_value {
        int value;
        explicit number_integer_t(int v) : value(v) {}
        value_t type() const override { return value_t::number_integer; }
        std::shared_ptr<json_value> clone() const override { return std::make_shared<number_integer_t>(value); }
    };

    struct number_float_t : json_value {
        double value;
        explicit number_float_t(double v) : value(v) {}
        value_t type() const override { return value_t::number_float; }
        std::shared_ptr<json_value> clone() const override { return std::make_shared<number_float_t>(value); }
    };

    struct array_t : json_value {
        std::vector<json> value;
        explicit array_t(std::vector<json> v) : value(std::move(v)) {}
        value_t type() const override { return value_t::array; }
        std::shared_ptr<json_value> clone() const override { return std::make_shared<array_t>(value); }
    };

    struct object_t : json_value {
        std::map<std::string, json> value;
        explicit object_t(std::map<std::string, json> v) : value(std::move(v)) {}
        value_t type() const override { return value_t::object; }
        std::shared_ptr<json_value> clone() const override { return std::make_shared<object_t>(value); }
    };

    std::shared_ptr<json_value> m_value;

public:
    // 默认构造函数，创建null值
    json() : m_value(std::make_shared<null_t>()) {}

    // 从不同类型构造
    json(std::nullptr_t) : m_value(std::make_shared<null_t>()) {}
    json(bool v) : m_value(std::make_shared<boolean_t>(v)) {}
    json(int v) : m_value(std::make_shared<number_integer_t>(v)) {}
    json(double v) : m_value(std::make_shared<number_float_t>(v)) {}
    json(const std::string& v) : m_value(std::make_shared<string_t>(v)) {}
    json(const char* v) : m_value(std::make_shared<string_t>(v)) {}
    json(std::initializer_list<json> init) : m_value(std::make_shared<array_t>(std::vector<json>(init))) {}
    
    // 拷贝/移动构造
    json(const json& other) : m_value(other.m_value->clone()) {}
    json(json&& other) noexcept : m_value(std::move(other.m_value)) {}

    // 赋值操作符
    json& operator=(const json& other) {
        if (this != &other) {
            m_value = other.m_value->clone();
        }
        return *this;
    }
    
    json& operator=(json&& other) noexcept {
        if (this != &other) {
            m_value = std::move(other.m_value);
        }
        return *this;
    }    // 类型转换操作符
    template<typename T>
    T get() const {
        throw std::domain_error("type mismatch");
    }
    
    // 模板特化必须在类外部或命名空间级别进行
    bool get_boolean() const {
        if (m_value->type() != value_t::boolean) {
            throw std::domain_error("type mismatch");
        }
        return static_cast<boolean_t*>(m_value.get())->value;
    }
    
    int get_integer() const {
        if (m_value->type() != value_t::number_integer) {
            throw std::domain_error("type mismatch");
        }
        return static_cast<number_integer_t*>(m_value.get())->value;
    }
    
    double get_double() const {
        if (m_value->type() == value_t::number_float) {
            return static_cast<number_float_t*>(m_value.get())->value;
        }
        if (m_value->type() == value_t::number_integer) {
            return static_cast<number_integer_t*>(m_value.get())->value;
        }
        throw std::domain_error("type mismatch");
    }
    
    std::string get_string() const {
        if (m_value->type() != value_t::string) {
            throw std::domain_error("type mismatch");
        }
        return static_cast<string_t*>(m_value.get())->value;
    }

    // 检查类型
    bool is_null() const { return m_value->type() == value_t::null; }
    bool is_boolean() const { return m_value->type() == value_t::boolean; }
    bool is_number() const { return m_value->type() == value_t::number_integer || m_value->type() == value_t::number_float; }
    bool is_string() const { return m_value->type() == value_t::string; }
    bool is_array() const { return m_value->type() == value_t::array; }
    bool is_object() const { return m_value->type() == value_t::object; }

    // 数组操作
    json& operator[](size_t idx) {
        if (m_value->type() != value_t::array) {
            m_value = std::make_shared<array_t>(std::vector<json>{});
        }
        auto& arr = static_cast<array_t*>(m_value.get())->value;
        if (idx >= arr.size()) {
            arr.resize(idx + 1);
        }
        return arr[idx];
    }
    
    // 对象操作
    json& operator[](const std::string& key) {
        if (m_value->type() != value_t::object) {
            m_value = std::make_shared<object_t>(std::map<std::string, json>{});
        }
        return static_cast<object_t*>(m_value.get())->value[key];
    }

    bool contains(const std::string& key) const {
        if (m_value->type() != value_t::object) {
            return false;
        }
        const auto& obj = static_cast<object_t*>(m_value.get())->value;
        return obj.find(key) != obj.end();
    }

    // JSON 序列化
    std::string dump(int indent = -1) const {
        switch (m_value->type()) {
            case value_t::null:
                return "null";
            case value_t::boolean:
                return static_cast<boolean_t*>(m_value.get())->value ? "true" : "false";
            case value_t::number_integer:
                return std::to_string(static_cast<number_integer_t*>(m_value.get())->value);
            case value_t::number_float:
                return std::to_string(static_cast<number_float_t*>(m_value.get())->value);
            case value_t::string:
                return "\"" + static_cast<string_t*>(m_value.get())->value + "\"";
            case value_t::array: {
                std::string result = "[";
                const auto& arr = static_cast<array_t*>(m_value.get())->value;
                for (size_t i = 0; i < arr.size(); ++i) {
                    if (i > 0) result += ",";
                    result += arr[i].dump();
                }
                result += "]";
                return result;
            }
            case value_t::object: {
                std::string result = "{";
                const auto& obj = static_cast<object_t*>(m_value.get())->value;
                bool first = true;
                for (const auto& pair : obj) {
                    if (!first) result += ",";
                    first = false;
                    result += "\"" + pair.first + "\":" + pair.second.dump();
                }
                result += "}";
                return result;
            }
            default:
                return "";
        }
    }

    // JSON 解析
    static json parse(const std::string& str) {
        // 这里只提供一个简单的占位实现
        // 实际项目中应该使用完整的JSON解析库
        json result;
        if (str.empty()) {
            return result;
        }
        
        // 非常简单的检测
        if (str[0] == '{') {
            result = json::object();
        } else if (str[0] == '[') {
            result = json::array();
        } else if (str == "null") {
            result = nullptr;
        } else if (str == "true") {
            result = true;
        } else if (str == "false") {
            result = false;
        } else if (str[0] == '"') {
            // 假设是字符串
            result = str.substr(1, str.length() - 2);
        } else {
            // 假设是数字
            try {
                result = std::stoi(str);
            } catch (...) {
                try {
                    result = std::stod(str);
                } catch (...) {
                    // 解析错误
                }
            }
        }
        
        return result;
    }

    // 构造辅助函数
    static json object() {
        json j;
        j.m_value = std::make_shared<object_t>(std::map<std::string, json>{});
        return j;
    }
    
    static json array() {
        json j;
        j.m_value = std::make_shared<array_t>(std::vector<json>{});
        return j;    }
};

// 在类外部定义模板特化
template<>
inline bool json::get<bool>() const {
    return get_boolean();
}

template<>
inline int json::get<int>() const {
    return get_integer();
}

template<>
inline double json::get<double>() const {
    return get_double();
}

template<>
inline std::string json::get<std::string>() const {
    return get_string();
}

} // namespace nlohmann

#endif // JSON_HPP
