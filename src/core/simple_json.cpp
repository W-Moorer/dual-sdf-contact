#include "baseline/core/simple_json.h"

#include <cstdlib>
#include <cctype>
#include <cmath>
#include <sstream>

#include "baseline/core/io_utils.h"

namespace baseline::json {

Value::Value(std::nullptr_t) : value_(nullptr) {}
Value::Value(bool value) : value_(value) {}
Value::Value(double value) : value_(value) {}
Value::Value(int value) : value_(static_cast<double>(value)) {}
Value::Value(std::string value) : value_(std::move(value)) {}
Value::Value(const char* value) : value_(std::string(value)) {}
Value::Value(Array value) : value_(std::move(value)) {}
Value::Value(Object value) : value_(std::move(value)) {}

bool Value::isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Value::isBool() const { return std::holds_alternative<bool>(value_); }
bool Value::isNumber() const { return std::holds_alternative<double>(value_); }
bool Value::isString() const { return std::holds_alternative<std::string>(value_); }
bool Value::isArray() const { return std::holds_alternative<Array>(value_); }
bool Value::isObject() const { return std::holds_alternative<Object>(value_); }

bool Value::asBool() const { return std::get<bool>(value_); }
double Value::asNumber() const { return std::get<double>(value_); }
const std::string& Value::asString() const { return std::get<std::string>(value_); }
const Value::Array& Value::asArray() const { return std::get<Array>(value_); }
const Value::Object& Value::asObject() const { return std::get<Object>(value_); }
Value::Array& Value::asArray() { return std::get<Array>(value_); }
Value::Object& Value::asObject() { return std::get<Object>(value_); }

bool Value::hasKey(std::string_view key) const {
  if (!isObject()) {
    return false;
  }
  return asObject().find(std::string(key)) != asObject().end();
}

const Value& Value::at(std::string_view key) const { return asObject().at(std::string(key)); }

namespace {

class Parser {
 public:
  explicit Parser(std::string_view text) : text_(text) {}

  Value parseValue() {
    skipWhitespace();
    if (eof()) {
      throw error("Unexpected end of JSON input.");
    }
    const char current = peek();
    if (current == '{') {
      return parseObject();
    }
    if (current == '[') {
      return parseArray();
    }
    if (current == '"') {
      return Value(parseString());
    }
    if (current == 't') {
      expectKeyword("true");
      return Value(true);
    }
    if (current == 'f') {
      expectKeyword("false");
      return Value(false);
    }
    if (current == 'n') {
      expectKeyword("null");
      return Value(nullptr);
    }
    if (current == '-' || std::isdigit(static_cast<unsigned char>(current))) {
      return Value(parseNumber());
    }
    throw error("Unexpected token.");
  }

  void ensureConsumed() {
    skipWhitespace();
    if (!eof()) {
      throw error("Unexpected trailing characters.");
    }
  }

 private:
  ParseError error(const std::string& message) const {
    std::ostringstream stream;
    stream << message << " At offset " << offset_ << ".";
    return ParseError(stream.str());
  }

  bool eof() const { return offset_ >= text_.size(); }
  char peek() const { return text_[offset_]; }

  char consume() {
    if (eof()) {
      throw error("Unexpected end of JSON input.");
    }
    return text_[offset_++];
  }

  void skipWhitespace() {
    while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
      ++offset_;
    }
  }

  void expect(char expected) {
    if (consume() != expected) {
      throw error(std::string("Expected '") + expected + "'.");
    }
  }

  void expectKeyword(std::string_view keyword) {
    for (char character : keyword) {
      if (consume() != character) {
        throw error("Malformed JSON keyword.");
      }
    }
  }

  std::string parseString() {
    expect('"');
    std::string result;
    while (!eof()) {
      const char current = consume();
      if (current == '"') {
        return result;
      }
      if (current != '\\') {
        result.push_back(current);
        continue;
      }
      if (eof()) {
        throw error("Unexpected end of escape sequence.");
      }
      const char escaped = consume();
      switch (escaped) {
        case '"':
        case '\\':
        case '/':
          result.push_back(escaped);
          break;
        case 'b':
          result.push_back('\b');
          break;
        case 'f':
          result.push_back('\f');
          break;
        case 'n':
          result.push_back('\n');
          break;
        case 'r':
          result.push_back('\r');
          break;
        case 't':
          result.push_back('\t');
          break;
        case 'u':
          throw error("Unicode escapes are not supported by this lightweight parser.");
        default:
          throw error("Invalid escape sequence.");
      }
    }
    throw error("Unterminated string.");
  }

  double parseNumber() {
    const std::size_t start = offset_;
    if (peek() == '-') {
      ++offset_;
    }
    consumeDigits();
    if (!eof() && peek() == '.') {
      ++offset_;
      consumeDigits();
    }
    if (!eof() && (peek() == 'e' || peek() == 'E')) {
      ++offset_;
      if (!eof() && (peek() == '+' || peek() == '-')) {
        ++offset_;
      }
      consumeDigits();
    }
    const std::string token(text_.substr(start, offset_ - start));
    char* end_ptr = nullptr;
    const double value = std::strtod(token.c_str(), &end_ptr);
    if (end_ptr == nullptr || *end_ptr != '\0' || !std::isfinite(value)) {
      throw error("Invalid number.");
    }
    return value;
  }

  void consumeDigits() {
    if (eof() || !std::isdigit(static_cast<unsigned char>(peek()))) {
      throw error("Expected a digit.");
    }
    while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
      ++offset_;
    }
  }

  Value parseArray() {
    expect('[');
    skipWhitespace();
    Value::Array result;
    if (!eof() && peek() == ']') {
      consume();
      return Value(std::move(result));
    }
    while (true) {
      result.push_back(parseValue());
      skipWhitespace();
      if (!eof() && peek() == ']') {
        consume();
        return Value(std::move(result));
      }
      expect(',');
      skipWhitespace();
    }
  }

  Value parseObject() {
    expect('{');
    skipWhitespace();
    Value::Object result;
    if (!eof() && peek() == '}') {
      consume();
      return Value(std::move(result));
    }
    while (true) {
      skipWhitespace();
      if (eof() || peek() != '"') {
        throw error("Expected string key.");
      }
      const std::string key = parseString();
      skipWhitespace();
      expect(':');
      skipWhitespace();
      result.emplace(key, parseValue());
      skipWhitespace();
      if (!eof() && peek() == '}') {
        consume();
        return Value(std::move(result));
      }
      expect(',');
      skipWhitespace();
    }
  }

  std::string_view text_;
  std::size_t offset_{0};
};

std::string indentPrefix(int indent, int level) {
  return std::string(static_cast<std::size_t>(indent * level), ' ');
}

std::string stringifyImpl(const Value& value, int indent, int level) {
  if (value.isNull()) {
    return "null";
  }
  if (value.isBool()) {
    return value.asBool() ? "true" : "false";
  }
  if (value.isNumber()) {
    return formatDouble(value.asNumber());
  }
  if (value.isString()) {
    return quoteJson(value.asString());
  }
  if (value.isArray()) {
    if (value.asArray().empty()) {
      return "[]";
    }
    std::ostringstream stream;
    stream << "[\n";
    for (std::size_t index = 0; index < value.asArray().size(); ++index) {
      if (index > 0) {
        stream << ",\n";
      }
      stream << indentPrefix(indent, level + 1) << stringifyImpl(value.asArray()[index], indent, level + 1);
    }
    stream << "\n" << indentPrefix(indent, level) << "]";
    return stream.str();
  }

  if (value.asObject().empty()) {
    return "{}";
  }
  std::ostringstream stream;
  stream << "{\n";
  std::size_t index = 0;
  for (const auto& [key, child] : value.asObject()) {
    if (index++ > 0) {
      stream << ",\n";
    }
    stream << indentPrefix(indent, level + 1) << quoteJson(key) << ": "
           << stringifyImpl(child, indent, level + 1);
  }
  stream << "\n" << indentPrefix(indent, level) << "}";
  return stream.str();
}

}  // namespace

Value parse(std::string_view text) {
  Parser parser(text);
  Value value = parser.parseValue();
  parser.ensureConsumed();
  return value;
}

std::string stringify(const Value& value, int indent) { return stringifyImpl(value, indent, 0); }

}  // namespace baseline::json
