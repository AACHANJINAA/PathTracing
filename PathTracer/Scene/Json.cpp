#include "Json.h"

#include <cstdlib>

namespace
{
    class PARSER
    {
    public:
        explicit PARSER(const std::string& text) : _text(text) {}

        bool parse(JSON_VALUE& value, std::string& error)
        {
            skip_space();
            if (!parse_value(value, 0)) { error = _error; return false; }
            skip_space();
            if (_position != _text.size()) { fail("unexpected trailing characters"); error = _error; return false; }
            return true;
        }

    private:
        static constexpr int max_depth = 64;
        const std::string& _text;
        size_t _position = 0;
        std::string _error;

        bool fail(const std::string& message)
        {
            if (!_error.empty()) return false;
            size_t line = 1, column = 1;
            for (size_t index = 0; index < _position && index < _text.size(); ++index)
            {
                if (_text[index] == '\n') { ++line; column = 1; }
                else ++column;
            }
            _error = "line " + std::to_string(line) + ", column " + std::to_string(column) + ": " + message;
            return false;
        }

        void skip_space()
        {
            while (_position < _text.size())
            {
                const char c = _text[_position];
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++_position;
                else break;
            }
        }

        bool match(const char* word)
        {
            const size_t length = std::char_traits<char>::length(word);
            if (_text.compare(_position, length, word) != 0) return false;
            _position += length;
            return true;
        }

        bool parse_value(JSON_VALUE& value, int depth)
        {
            if (depth > max_depth) return fail("nesting too deep");
            if (_position >= _text.size()) return fail("unexpected end of input");
            const char c = _text[_position];
            if (c == '{') return parse_object(value, depth);
            if (c == '[') return parse_array(value, depth);
            if (c == '"') { value.type = JSON_VALUE::TYPE::string; return parse_string(value.string); }
            if (match("true")) { value.type = JSON_VALUE::TYPE::boolean; value.boolean = true; return true; }
            if (match("false")) { value.type = JSON_VALUE::TYPE::boolean; value.boolean = false; return true; }
            if (match("null")) { value.type = JSON_VALUE::TYPE::null; return true; }
            if (c == '-' || (c >= '0' && c <= '9')) return parse_number(value);
            return fail(std::string("unexpected character '") + c + "'");
        }

        bool parse_number(JSON_VALUE& value)
        {
            const char* begin = _text.c_str() + _position;
            char* end = nullptr;
            const double number = std::strtod(begin, &end);
            if (end == begin) return fail("invalid number");
            _position += static_cast<size_t>(end - begin);
            value.type = JSON_VALUE::TYPE::number;
            value.number = number;
            return true;
        }

        bool parse_string(std::string& out)
        {
            ++_position; // opening quote
            out.clear();
            while (_position < _text.size())
            {
                const char c = _text[_position++];
                if (c == '"') return true;
                if (c == '\\')
                {
                    if (_position >= _text.size()) break;
                    const char e = _text[_position++];
                    switch (e)
                    {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u':
                    {
                        if (_position + 4 > _text.size()) return fail("bad unicode escape");
                        const unsigned code = static_cast<unsigned>(std::strtoul(_text.substr(_position, 4).c_str(), nullptr, 16));
                        _position += 4;
                        // UTF-8 encode the BMP code point (surrogate pairs are not needed for scene files).
                        if (code < 0x80) out += static_cast<char>(code);
                        else if (code < 0x800) { out += static_cast<char>(0xC0 | (code >> 6)); out += static_cast<char>(0x80 | (code & 0x3F)); }
                        else { out += static_cast<char>(0xE0 | (code >> 12)); out += static_cast<char>(0x80 | ((code >> 6) & 0x3F)); out += static_cast<char>(0x80 | (code & 0x3F)); }
                        break;
                    }
                    default: return fail("bad escape sequence");
                    }
                }
                else out += c;
            }
            return fail("unterminated string");
        }

        bool parse_array(JSON_VALUE& value, int depth)
        {
            value.type = JSON_VALUE::TYPE::array;
            ++_position;
            skip_space();
            if (_position < _text.size() && _text[_position] == ']') { ++_position; return true; }
            while (true)
            {
                JSON_VALUE element;
                skip_space();
                if (!parse_value(element, depth + 1)) return false;
                value.array.push_back(std::move(element));
                skip_space();
                if (_position >= _text.size()) return fail("unterminated array");
                if (_text[_position] == ',') { ++_position; continue; }
                if (_text[_position] == ']') { ++_position; return true; }
                return fail("expected ',' or ']'");
            }
        }

        bool parse_object(JSON_VALUE& value, int depth)
        {
            value.type = JSON_VALUE::TYPE::object;
            ++_position;
            skip_space();
            if (_position < _text.size() && _text[_position] == '}') { ++_position; return true; }
            while (true)
            {
                skip_space();
                if (_position >= _text.size() || _text[_position] != '"') return fail("expected object key");
                std::string key;
                if (!parse_string(key)) return false;
                for (const auto& entry : value.object)
                    if (entry.first == key) return fail("duplicate key \"" + key + "\"");
                skip_space();
                if (_position >= _text.size() || _text[_position] != ':') return fail("expected ':'");
                ++_position;
                skip_space();
                JSON_VALUE element;
                if (!parse_value(element, depth + 1)) return false;
                value.object.emplace_back(std::move(key), std::move(element));
                skip_space();
                if (_position >= _text.size()) return fail("unterminated object");
                if (_text[_position] == ',') { ++_position; continue; }
                if (_text[_position] == '}') { ++_position; return true; }
                return fail("expected ',' or '}'");
            }
        }
    };
}

const JSON_VALUE* JSON_VALUE::find(const std::string& key) const
{
    for (const auto& entry : object)
        if (entry.first == key) return &entry.second;
    return nullptr;
}

bool parse_json(const std::string& text, JSON_VALUE& value, std::string& error)
{
    value = JSON_VALUE{};
    PARSER parser(text);
    return parser.parse(value, error);
}
