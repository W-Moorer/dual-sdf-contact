#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace baseline::json {

class ParseError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class Value {
 public:
  using Array = std::vector<Value>;
  using Object = std::map<std::string, Value>;

  Value() = default;
  Value(std::nullptr_t);
  Value(bool value);
  Value(double value);
  Value(int value);
  Value(std::string value);
  Value(const char* value);
  Value(Array value);
  Value(Object value);

  bool isNull() const;
  bool isBool() const;
  bool isNumber() const;
  bool isString() const;
  bool isArray() const;
  bool isObject() const;

  bool asBool() const;
  double asNumber() const;
  const std::string& asString() const;
  const Array& asArray() const;
  const Object& asObject() const;
  Array& asArray();
  Object& asObject();

  bool hasKey(std::string_view key) const;
  const Value& at(std::string_view key) const;

 private:
  std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value_{nullptr};
};

Value parse(std::string_view text);
std::string stringify(const Value& value, int indent = 2);

}  // namespace baseline::json
