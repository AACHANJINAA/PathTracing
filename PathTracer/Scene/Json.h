#pragma once

// Minimal JSON reader for scene files (no external dependency, portable C++).
// Objects keep key order so error messages and round trips stay predictable.

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class JSON_VALUE
{
public:
    enum class TYPE { null, boolean, number, string, array, object };

    TYPE type = TYPE::null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::vector<JSON_VALUE> array;
    std::vector<std::pair<std::string, JSON_VALUE>> object;

    bool is_object() const { return type == TYPE::object; }
    bool is_array() const { return type == TYPE::array; }
    bool is_number() const { return type == TYPE::number; }
    bool is_string() const { return type == TYPE::string; }
    bool is_boolean() const { return type == TYPE::boolean; }
    const JSON_VALUE* find(const std::string& key) const;
};

// Returns false and fills error ("line L, column C: message") on malformed input.
bool parse_json(const std::string& text, JSON_VALUE& value, std::string& error);
