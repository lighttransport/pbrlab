#include "json.h"
#include <cctype>
#include <iomanip>

namespace pbrlab {
namespace mcp {
namespace json {

std::string JsonValue::stringify() const {
  return JsonBuilder::stringify(*this);
}

JsonValue JsonParser::parse(const std::string& json) {
  JsonParser parser(json);
  return parser.parseValue();
}

JsonValue JsonParser::parseValue() {
  skipWhitespace();

  char c = peek();
  if (c == '{') return parseObject();
  if (c == '[') return parseArray();
  if (c == '"') return parseString();
  if (c == 't' || c == 'f') return parseBoolean();
  if (c == 'n') return parseNull();
  if (c == '-' || std::isdigit(c)) return parseNumber();

  throw std::runtime_error("Invalid JSON at position " + std::to_string(pos_));
}

JsonValue JsonParser::parseObject() {
  consume('{');
  skipWhitespace();

  JsonObject obj;

  if (peek() == '}') {
    consume('}');
    return JsonValue(obj);
  }

  while (true) {
    skipWhitespace();
    if (peek() != '"') {
      throw std::runtime_error("Expected string key in object");
    }

    std::string key = parseStringLiteral();
    skipWhitespace();

    if (!consume(':')) {
      throw std::runtime_error("Expected ':' after object key");
    }

    skipWhitespace();
    obj[key] = parseValue();
    skipWhitespace();

    if (peek() == '}') {
      consume('}');
      break;
    }

    if (!consume(',')) {
      throw std::runtime_error("Expected ',' or '}' in object");
    }
  }

  return JsonValue(obj);
}

JsonValue JsonParser::parseArray() {
  consume('[');
  skipWhitespace();

  JsonArray arr;

  if (peek() == ']') {
    consume(']');
    return JsonValue(arr);
  }

  while (true) {
    skipWhitespace();
    arr.push_back(parseValue());
    skipWhitespace();

    if (peek() == ']') {
      consume(']');
      break;
    }

    if (!consume(',')) {
      throw std::runtime_error("Expected ',' or ']' in array");
    }
  }

  return JsonValue(arr);
}

JsonValue JsonParser::parseString() {
  return JsonValue(parseStringLiteral());
}

std::string JsonParser::parseStringLiteral() {
  consume('"');
  std::string result;

  while (pos_ < json_.size() && json_[pos_] != '"') {
    if (json_[pos_] == '\\') {
      pos_++;
      if (pos_ >= json_.size()) {
        throw std::runtime_error("Unexpected end of string");
      }

      switch (json_[pos_]) {
        case '"': result += '"'; break;
        case '\\': result += '\\'; break;
        case '/': result += '/'; break;
        case 'b': result += '\b'; break;
        case 'f': result += '\f'; break;
        case 'n': result += '\n'; break;
        case 'r': result += '\r'; break;
        case 't': result += '\t'; break;
        case 'u': {
          // Unicode escape sequence (simplified)
          pos_++;
          int code = 0;
          for (int i = 0; i < 4; i++) {
            if (pos_ >= json_.size()) {
              throw std::runtime_error("Invalid unicode escape");
            }
            char c = json_[pos_++];
            code = code * 16 + (std::isdigit(c) ? c - '0' :
                               std::tolower(c) - 'a' + 10);
          }
          pos_--; // Adjust for the loop increment
          if (code < 128) {
            result += static_cast<char>(code);
          } else {
            // Simplified UTF-8 encoding for ASCII range only
            result += '?';
          }
          break;
        }
        default:
          throw std::runtime_error("Invalid escape sequence");
      }
      pos_++;
    } else {
      result += json_[pos_++];
    }
  }

  if (pos_ >= json_.size() || json_[pos_] != '"') {
    throw std::runtime_error("Unterminated string");
  }

  consume('"');
  return result;
}

JsonValue JsonParser::parseNumber() {
  size_t start = pos_;

  if (json_[pos_] == '-') pos_++;

  if (pos_ >= json_.size() || !std::isdigit(json_[pos_])) {
    throw std::runtime_error("Invalid number");
  }

  while (pos_ < json_.size() && std::isdigit(json_[pos_])) {
    pos_++;
  }

  if (pos_ < json_.size() && json_[pos_] == '.') {
    pos_++;
    if (pos_ >= json_.size() || !std::isdigit(json_[pos_])) {
      throw std::runtime_error("Invalid number");
    }
    while (pos_ < json_.size() && std::isdigit(json_[pos_])) {
      pos_++;
    }
  }

  if (pos_ < json_.size() && (json_[pos_] == 'e' || json_[pos_] == 'E')) {
    pos_++;
    if (pos_ < json_.size() && (json_[pos_] == '+' || json_[pos_] == '-')) {
      pos_++;
    }
    if (pos_ >= json_.size() || !std::isdigit(json_[pos_])) {
      throw std::runtime_error("Invalid number");
    }
    while (pos_ < json_.size() && std::isdigit(json_[pos_])) {
      pos_++;
    }
  }

  std::string numStr = json_.substr(start, pos_ - start);
  return JsonValue(std::stod(numStr));
}

JsonValue JsonParser::parseBoolean() {
  if (json_.substr(pos_, 4) == "true") {
    pos_ += 4;
    return JsonValue(true);
  }
  if (json_.substr(pos_, 5) == "false") {
    pos_ += 5;
    return JsonValue(false);
  }
  throw std::runtime_error("Invalid boolean");
}

JsonValue JsonParser::parseNull() {
  if (json_.substr(pos_, 4) == "null") {
    pos_ += 4;
    return JsonValue::null();
  }
  throw std::runtime_error("Invalid null");
}

void JsonParser::skipWhitespace() {
  while (pos_ < json_.size() && std::isspace(json_[pos_])) {
    pos_++;
  }
}

char JsonParser::peek() const {
  if (pos_ >= json_.size()) {
    throw std::runtime_error("Unexpected end of JSON");
  }
  return json_[pos_];
}

char JsonParser::consume() {
  if (pos_ >= json_.size()) {
    throw std::runtime_error("Unexpected end of JSON");
  }
  return json_[pos_++];
}

bool JsonParser::consume(char expected) {
  if (pos_ < json_.size() && json_[pos_] == expected) {
    pos_++;
    return true;
  }
  return false;
}

std::string JsonBuilder::stringify(const JsonValue& value, bool pretty, int indent) {
  std::stringstream ss;
  stringifyValue(ss, value, pretty, indent);
  return ss.str();
}

void JsonBuilder::stringifyValue(std::stringstream& ss, const JsonValue& value,
                                 bool pretty, int indent) {
  switch (value.type()) {
    case JsonType::Null:
      ss << "null";
      break;
    case JsonType::Boolean:
      ss << (value.asBool() ? "true" : "false");
      break;
    case JsonType::Number:
      ss << value.asNumber();
      break;
    case JsonType::String:
      stringifyString(ss, value.asString());
      break;
    case JsonType::Array:
      stringifyArray(ss, value.asArray(), pretty, indent);
      break;
    case JsonType::Object:
      stringifyObject(ss, value.asObject(), pretty, indent);
      break;
  }
}

void JsonBuilder::stringifyObject(std::stringstream& ss, const JsonObject& obj,
                                  bool pretty, int indent) {
  ss << "{";
  if (pretty && !obj.empty()) ss << "\n";

  bool first = true;
  for (const auto& pair : obj) {
    if (!first) {
      ss << ",";
      if (pretty) ss << "\n";
    }
    first = false;

    if (pretty) addIndent(ss, indent + 2);
    stringifyString(ss, pair.first);
    ss << ":";
    if (pretty) ss << " ";
    stringifyValue(ss, pair.second, pretty, indent + 2);
  }

  if (pretty && !obj.empty()) {
    ss << "\n";
    addIndent(ss, indent);
  }
  ss << "}";
}

void JsonBuilder::stringifyArray(std::stringstream& ss, const JsonArray& arr,
                                 bool pretty, int indent) {
  ss << "[";
  if (pretty && !arr.empty()) ss << "\n";

  for (size_t i = 0; i < arr.size(); i++) {
    if (i > 0) {
      ss << ",";
      if (pretty) ss << "\n";
    }
    if (pretty) addIndent(ss, indent + 2);
    stringifyValue(ss, arr[i], pretty, indent + 2);
  }

  if (pretty && !arr.empty()) {
    ss << "\n";
    addIndent(ss, indent);
  }
  ss << "]";
}

void JsonBuilder::stringifyString(std::stringstream& ss, const std::string& str) {
  ss << "\"";
  for (char c : str) {
    switch (c) {
      case '"': ss << "\\\""; break;
      case '\\': ss << "\\\\"; break;
      case '\b': ss << "\\b"; break;
      case '\f': ss << "\\f"; break;
      case '\n': ss << "\\n"; break;
      case '\r': ss << "\\r"; break;
      case '\t': ss << "\\t"; break;
      default:
        if (c >= 0x20 && c <= 0x7E) {
          ss << c;
        } else {
          ss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
             << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
  }
  ss << "\"";
}

void JsonBuilder::addIndent(std::stringstream& ss, int indent) {
  for (int i = 0; i < indent; i++) {
    ss << " ";
  }
}

} // namespace json
} // namespace mcp
} // namespace pbrlab