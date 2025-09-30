#ifndef PBRLAB_MCP_JSON_H_
#define PBRLAB_MCP_JSON_H_

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <cstdint>

namespace pbrlab {
namespace mcp {
namespace json {

class JsonValue;
using JsonObject = std::map<std::string, JsonValue>;
using JsonArray = std::vector<JsonValue>;

enum class JsonType {
  Null,
  Boolean,
  Number,
  String,
  Array,
  Object
};

class JsonValue {
public:
  JsonValue() : type_(JsonType::Null) {}
  explicit JsonValue(bool b) : type_(JsonType::Boolean), bool_value_(b) {}
  explicit JsonValue(int i) : type_(JsonType::Number), number_value_(static_cast<double>(i)) {}
  explicit JsonValue(double d) : type_(JsonType::Number), number_value_(d) {}
  explicit JsonValue(const char* s) : type_(JsonType::String), string_value_(s) {}
  explicit JsonValue(const std::string& s) : type_(JsonType::String), string_value_(s) {}
  explicit JsonValue(const JsonArray& a) : type_(JsonType::Array), array_value_(a) {}
  explicit JsonValue(const JsonObject& o) : type_(JsonType::Object), object_value_(o) {}

  static JsonValue null() { return JsonValue(); }
  static JsonValue array() { return JsonValue(JsonArray()); }
  static JsonValue object() { return JsonValue(JsonObject()); }

  JsonType type() const { return type_; }
  bool isNull() const { return type_ == JsonType::Null; }
  bool isBool() const { return type_ == JsonType::Boolean; }
  bool isNumber() const { return type_ == JsonType::Number; }
  bool isString() const { return type_ == JsonType::String; }
  bool isArray() const { return type_ == JsonType::Array; }
  bool isObject() const { return type_ == JsonType::Object; }

  bool asBool() const {
    if (!isBool()) throw std::runtime_error("Not a boolean");
    return bool_value_;
  }

  double asNumber() const {
    if (!isNumber()) throw std::runtime_error("Not a number");
    return number_value_;
  }

  int asInt() const {
    if (!isNumber()) throw std::runtime_error("Not a number");
    return static_cast<int>(number_value_);
  }

  const std::string& asString() const {
    if (!isString()) throw std::runtime_error("Not a string");
    return string_value_;
  }

  JsonArray& asArray() {
    if (!isArray()) throw std::runtime_error("Not an array");
    return array_value_;
  }

  const JsonArray& asArray() const {
    if (!isArray()) throw std::runtime_error("Not an array");
    return array_value_;
  }

  JsonObject& asObject() {
    if (!isObject()) throw std::runtime_error("Not an object");
    return object_value_;
  }

  const JsonObject& asObject() const {
    if (!isObject()) throw std::runtime_error("Not an object");
    return object_value_;
  }

  // Array operations
  void push(const JsonValue& v) {
    if (!isArray()) throw std::runtime_error("Not an array");
    array_value_.push_back(v);
  }

  JsonValue& operator[](size_t index) {
    if (!isArray()) throw std::runtime_error("Not an array");
    return array_value_[index];
  }

  const JsonValue& operator[](size_t index) const {
    if (!isArray()) throw std::runtime_error("Not an array");
    return array_value_[index];
  }

  // Object operations
  JsonValue& operator[](const std::string& key) {
    if (!isObject()) throw std::runtime_error("Not an object");
    return object_value_[key];
  }

  const JsonValue& at(const std::string& key) const {
    if (!isObject()) throw std::runtime_error("Not an object");
    auto it = object_value_.find(key);
    if (it == object_value_.end()) {
      throw std::runtime_error("Key not found: " + key);
    }
    return it->second;
  }

  bool has(const std::string& key) const {
    if (!isObject()) return false;
    return object_value_.find(key) != object_value_.end();
  }

  size_t size() const {
    if (isArray()) return array_value_.size();
    if (isObject()) return object_value_.size();
    return 0;
  }

  std::string stringify() const;

private:
  JsonType type_;
  bool bool_value_ = false;
  double number_value_ = 0.0;
  std::string string_value_;
  JsonArray array_value_;
  JsonObject object_value_;
};

class JsonParser {
public:
  static JsonValue parse(const std::string& json);

private:
  JsonParser(const std::string& json) : json_(json), pos_(0) {}

  JsonValue parseValue();
  JsonValue parseObject();
  JsonValue parseArray();
  JsonValue parseString();
  JsonValue parseNumber();
  JsonValue parseBoolean();
  JsonValue parseNull();

  void skipWhitespace();
  char peek() const;
  char consume();
  bool consume(char expected);
  std::string parseStringLiteral();

  const std::string& json_;
  size_t pos_;
};

class JsonBuilder {
public:
  static std::string stringify(const JsonValue& value, bool pretty = false, int indent = 0);

private:
  static void stringifyValue(std::stringstream& ss, const JsonValue& value, bool pretty, int indent);
  static void stringifyObject(std::stringstream& ss, const JsonObject& obj, bool pretty, int indent);
  static void stringifyArray(std::stringstream& ss, const JsonArray& arr, bool pretty, int indent);
  static void stringifyString(std::stringstream& ss, const std::string& str);
  static void addIndent(std::stringstream& ss, int indent);
};

} // namespace json
} // namespace mcp
} // namespace pbrlab

#endif // PBRLAB_MCP_JSON_H_