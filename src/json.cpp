#include "json.h"
#include <cerrno>
#include <cmath>
#include <sstream>
#include <climits>

namespace json {
    Type::Type() : type{JSON_NULL} {}

    Type::Type(const Type& other) : type{JSON_NULL} {
        *this = other;
    }

    Type& Type::operator=(const Type& other) {
        if (this == &other) {
            return *this;
        }
        free();
        type = other.type;
        switch(other.type) {
            case OBJECT:
                object = new JsonObject(*other.object);
                break;
            case LIST:
                list = new JsonList(*other.list);
                break;
            case INTEGER:
                integer = other.integer;
                break;
            case DOUBLE:
                decimal = other.decimal;
                break;
            case BOOL:
                boolean = other.boolean;
                break;
            case STRING:
                str = new std::string(*other.str);
                break;
            case JSON_NULL:
                break;
        }
        return *this;
    }

    Type::Type(Type&& other) : type{JSON_NULL} {
        *this = std::move(other);
    }

    Type& Type::operator=(Type&& other) {
        if (this == &other) {
            return *this;
        }
        free();
        type = other.type;
        other.type = JSON_NULL;
        switch(type) {
            case OBJECT:
                object = other.object;
                other.object = nullptr;
                break;
            case LIST:
                list = other.list;
                other.list = nullptr;
                break;
            case INTEGER:
                integer = other.integer;
                break;
            case DOUBLE:
                decimal = other.decimal;
                break;
            case BOOL:
                boolean = other.boolean;
                break;
            case STRING:
                str = other.str;
                other.str = nullptr;
                break;
            case JSON_NULL:
                break;
        }
        return *this;
    }

    Type::~Type() {
        free();
        type = JSON_NULL;
    }
    template <> const JsonObject *Type::get() const {
        if (type == OBJECT) {
            return object;
        }
        return nullptr;
    }
    template <> const JsonList *Type::get() const {
        if (type == LIST) {
            return list;
        }
        return nullptr;
    }
    template <> const int64_t *Type::get() const {
        if (type == INTEGER) {
            return &integer;
        }
        return nullptr;
    }
    template <> const double *Type::get() const {
        if (type == DOUBLE) {
            return &decimal;
        }
        return nullptr;
    }
    template <> const bool *Type::get() const {
        if (type == BOOL) {
            return &boolean;
        }
        return nullptr;
    }
    template <> const std::string* Type::get() const {
        if (type == STRING) {
            return str;
        }
        return nullptr;
    }
    template <> void Type::set(JsonObject obj) {
        free();
        type = OBJECT;
        object = new JsonObject(std::move(obj));
    }
    template <> void Type::set(JsonList jlist) {
        free();
        type = LIST;
        list = new JsonList(std::move(jlist));
    }
    template <> void Type::set(int64_t i) {
        free();
        type = INTEGER;
        integer = i;
    }
    template <> void Type::set(double d) {
        free();
        type = DOUBLE;
        decimal = d;
    }
    template <> void Type::set(bool b) {
        free();
        type = BOOL;
        boolean = b;
    }
    template <> void Type::set(std::string s) {
        free();
        type = STRING;
        str = new std::string(std::move(s));
    }
    template <> void Type::set(std::nullptr_t n) {
        free();
        type = JSON_NULL;
    }

    void Type::free() {
        switch (type) {
        case OBJECT:
            delete object;
            break;
        case STRING: 
            delete str;
            break;
        case LIST:
            delete list;
            break;
        default:
            break;
        }
    }
}

void get_position(std::size_t ix, const std::string &s, std::size_t &row,
                  std::size_t &col) {
    row = 1;
    col = 1;
    for (std::size_t i = 0; i < ix; ++i) {
        if (s[i] == '\n') {
            ++row;
            col = 1;
        } else {
            ++col;
        }
    }
}

json_exception expected_char(std::size_t ix, const std::string &s, char c) {
    std::size_t row, col;
    get_position(ix, s, row, col);
    return json_exception("Expected a '" + std::string(1, c) + "' at row " +
                          std::to_string(row) + " , col " +
                          std::to_string(col));
}

json_exception unexpected_char(std::size_t ix, const std::string &s, char c) {
    std::size_t row, col;
    get_position(ix, s, row, col);
    return json_exception("Unexpected character '" + std::string(1, c) +
                          "' at row " + std::to_string(row) + " , col " +
                          std::to_string(col));
}

json_exception to_big_number(std::size_t ix, const std::string &s) {
    std::size_t row, col;
    get_position(ix, s, row, col);
    return json_exception("To big number at row " + std::to_string(row) +
                          " , col " + std::to_string(col));
}

json_exception end_of_data() {
    return json_exception("Unexpected end of data");
}

void skip_spacing(std::size_t &ix, const std::string &in);

void validate_char(std::size_t ix, const std::string &in, char c);

JsonObject read_object(std::size_t &ix, const std::string &in);

JsonList read_list(std::size_t &ix, const std::string &in);

void read_matching(std::size_t &ix, const std::string &in, const std::string &s);

bool read_number(std::size_t &ix, const std::string &in, int64_t &i_val,
                 double &d_val);

void to_pretty_stream(std::ostream &os, const json::Type &val,
                      int indentations);

std::string read_string(std::size_t &ix, const std::string &in);

void read_matching(std::size_t &ix, const std::string &in, const std::string &s) {
    if (ix >= in.size()) {
        throw end_of_data();
    }
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] != in[ix]) {
            throw unexpected_char(ix, in, in[ix]);
        }
        ++ix;
        if (ix >= in.size()) {
            throw end_of_data();
        }
    }
    --ix;
}

void validate_char(std::size_t ix, const std::string &s, char c) {
    if (ix >= s.size()) {
        throw end_of_data();
    }
    if (s[ix] != c) {
        throw expected_char(ix, s, c);
    }
}

bool read_number(std::size_t &ix, const std::string &in, int64_t &i_val,
                 double &d_val) {
    bool is_int = true;
    bool negative = false;
    if (ix >= in.size()) {
        throw end_of_data();
    }
    if (in[ix] == '-') {
        negative = true;
        if (++ix >= in.size()) {
            throw end_of_data();
        }
    }
    if (in[ix] == '0') {
        if (++ix >= in.size() || in[ix] != '.') {
            i_val = 0;
            --ix;
            return true;
        }
        is_int = false;
        if (++ix >= in.size()) {
            throw end_of_data();
        }
    }
    if (in[ix] - '0' > 9 || in[ix] - '0' < 0) {
        throw unexpected_char(ix, in, in[ix]);
    }
    i_val = 0;
    d_val = 0;
    double pos = 1;
    while (true) {
        if (in[ix] - '0' <= 9 && in[ix] - '0' >= 0) {
            if (is_int) {
                if (i_val > INT_MAX / 10) {
                    throw to_big_number(ix, in);
                }
                i_val *= 10;
                i_val += (in[ix] - '0');
            } else {
                pos *= 10;
                d_val += (in[ix] - '0') / pos;
            }
        } else if (in[ix] == '.') {
            is_int = false;
            d_val = (double)i_val;
        } else if (in[ix] == 'E' || in[ix] == 'e') {
            if (++ix >= in.size()) {
                throw end_of_data();
            }
            bool negate_exp = false;
            if (in[ix] == '-') {
                negate_exp = true;
            }
            if (in[ix] == '+' || in[ix] == '-') {
                if (++ix >= in.size()) {
                    throw end_of_data();
                }
            }
            double exp = 0;
            int exp_pos = 1;
            // Number with exponent is considered double.
            if (in[ix] - '0' > 9 || in[ix] - '0' < 0) {
                throw unexpected_char(ix, in, in[ix]);
            }
            while (in[ix] - '0' <= 9 && in[ix] - '0' >= 0) {
                exp *= exp_pos;
                exp += (in[ix] - '0');
                exp_pos *= 10;
                if (++ix >= in.size()) {
                    break;
                }
            }
            if (negative) {
                d_val *= -1;
            }
            --ix;
            errno = 0;
            double factor = std::pow(10.0, exp);
            if (errno == ERANGE) {
                throw to_big_number(ix, in);
            }
            d_val = d_val * factor;
            return false;
        } else {
            --ix;
            if (negative) {
                i_val *= -1;
                d_val *= -1;
            }
            return is_int;
        }
        if (++ix >= in.size()) {
            --ix;
            if (negative) {
                i_val *= -1;
                d_val *= -1;
            }
            return is_int;
        }
    }
}

JsonObject read_object(std::size_t &ix, const std::string &in) {
    validate_char(ix, in, '{');
    JsonObject obj;
    skip_spacing(ix, in);
    if (ix >= in.size()) {
        throw end_of_data();
    }
    if (in[ix] == '}') {
        return obj;
    }
    while (true) {
        std::string key = read_string(ix, in);
        skip_spacing(ix, in);
        validate_char(ix, in, ':');
        skip_spacing(ix, in);
        if (ix >= in.size()) {
            throw end_of_data();
        }
        switch (in[ix]) {
        case '"': {
            std::string val = read_string(ix, in);
            obj.set(key, val);
            break;
        }
        case '{': {
            JsonObject val = read_object(ix, in);
            obj.set(key, val);
            break;
        }
        case '[': {
            JsonList val = read_list(ix, in);
            obj.set(key, val);
            break;
        }
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            int64_t i_val;
            double d_val;
            if (read_number(ix, in, i_val, d_val)) {
                obj.set(key, i_val);
            } else {
                obj.set(key, d_val);
            }
            break;
        }
        case 'n': {
            read_matching(ix, in, "null");
            obj.set<std::nullptr_t>(key, nullptr);
            break;
        }
        case 't': {
            read_matching(ix, in, "true");
            obj.set<bool>(key, true);
            break;
        }
        case 'f': {
            read_matching(ix, in, "false");
            obj.set<bool>(key, false);
            break;
        }
        default:
            throw unexpected_char(ix, in, in[ix]);
        }
        skip_spacing(ix, in);
        if (ix >= in.size()) {
            throw end_of_data();
        }
        if (in[ix] == '}') {
            return obj;
        }
        if (in[ix] != ',') {
            throw unexpected_char(ix, in, in[ix]);
        }
        skip_spacing(ix, in);
    }
}

JsonList read_list(std::size_t& ix, const std::string& in) {
    validate_char(ix, in, '[');
    JsonList list;
    skip_spacing(ix, in);
    if (ix >= in.size()) {
        throw end_of_data();
    }
    if (in[ix] == ']') {
        return list;
    }
    while (true) {
        switch (in[ix]) {
        case '"': {
            std::string val = read_string(ix, in);
            list.push_back(val);
            break;
        }
        case '{': {
            JsonObject val = read_object(ix, in);
            list.push_back(val);
            break;
        }
        case '[': {
            JsonList val = read_list(ix, in);
            list.push_back(val);
            break;
        }
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            int64_t i_val;
            double d_val;
            if (read_number(ix, in, i_val, d_val)) {
                list.push_back(i_val);
            } else {
                list.push_back(d_val);
            }
            break;
        }
        case 'n': {
            read_matching(ix, in, "null");
            list.push_back<std::nullptr_t>(nullptr);
            break;
        }
        case 't': {
            read_matching(ix, in, "true");
            list.push_back<bool>(true);

            break;
        }
        case 'f': {
            read_matching(ix, in, "false");
            list.push_back<bool>(false);
            break;
        }
        default:
            throw unexpected_char(ix, in, in[ix]);
        }
        skip_spacing(ix, in);
        if (ix >= in.size()) {
            throw end_of_data();
        }
        if (in[ix] == ']') {
            return list;
        }
        else if (in[ix] != ',') {
            throw unexpected_char(ix, in, in[ix]);
        }
        skip_spacing(ix, in);
        if (ix >= in.size()) {
            throw end_of_data();
        }
    }
}

std::string read_string(std::size_t& ix, const std::string& in) {
    validate_char(ix, in, '"');
    std::string res;
    while (++ix < in.size()) {
        if (in[ix] == '\\') {
            if (++ix >= in.size()) {
                throw end_of_data();
            }
            if (in[ix] == '\\' || in[ix] == '"' || in[ix] == '/') {
                res.push_back(in[ix]);
            } else if (in[ix] == 'n') {
                res.push_back('\n');
            } else if (in[ix] == 't') {
                res.push_back('\t');
            } else if (in[ix] == 'b') {
                res.push_back('\b');
            } else if (in[ix] == 'f') {
                res.push_back('\f');
            } else if (in[ix] == 'r') {
                res.push_back('\r');
            } else {
                throw json_exception("Unsupported escape character: \\" +
                                     std::string(1, in[ix]));
            }
        } else if (in[ix] == '"') {
            return res;
        } else {
            res.push_back(in[ix]);
        }
    }
    throw end_of_data();
}

void skip_spacing(std::size_t &ix, const std::string &in) {
    do {
        ++ix;
    } while (ix < in.size() && (in[ix] == ' ' || in[ix] == '\n' ||
                                in[ix] == '\r' || in[ix] == '\t'));
}

JsonObject json::read_from_string(const std::string& in) {
    std::size_t ix = 0;
    if (in[0] != '{') {
        skip_spacing(ix, in);
    }
    return read_object(ix, in);
}

void json::to_pretty_stream(std::ostream &os, const json::Type &val) {
    ::to_pretty_stream(os, val, 0);
}

void json::escape_string_to_stream(std::ostream &os, const std::string &s) {
    os << '"';
    for (char c : s) {
        switch (c) {
        case '\\':
            os << "\\\\";
            break;
        case '\"':
            os << "\\\"";
            break;
        case '\n':
            os << "\\n";
            break;
        case '\t':
            os << "\\t";
            break;
        case '\b':
            os << "\\b";
            break;
        case '\f':
            os << "\\f";
        case '\r':
            os << "\\r";
            break;
        default:
            os << c;
        }
    }
    os << '"';
}

void json::to_stream(std::ostream &os, const json::Type &val) {
    if (const JsonObject *obj = val.get<JsonObject>()) {
        obj->to_stream(os);
    } else if (const JsonList *list = val.get<JsonList>()) {
        list->to_stream(os);
    } else if (val.type == Type::JSON_NULL) {
        os << "null";
    } else if (const int64_t *i = val.get<int64_t>()) {
        os << *i;
    } else if (const double *d = val.get<double>()) {
        os << *d;
    } else if (const bool *b = val.get<bool>()) {
        if (*b)
            os << "true";
        else
            os << "false";
    } else if (const std::string *s = val.get<std::string>()) {
        json::escape_string_to_stream(os, *s);
    }
}

std::string json::write_to_string(const JsonObject &obj) {
    return write_to_string(obj, true);
}

std::string json::write_to_string(const JsonObject &obj, bool pretty) {
    std::stringstream s;
    if (pretty) {
        s << obj;
    } else {
        obj.to_stream(s);
    }
    return s.str();
}

void to_pretty_stream(std::ostream &os, const json::Type &val,
                      int indentations) {
    if (const JsonObject *obj = val.get<JsonObject>()) {
        obj->to_pretty_stream(os, indentations);
    } else if (const JsonList *list = val.get<JsonList>()) {
        list->to_pretty_stream(os, indentations);
    } else if (val.type == json::Type::JSON_NULL) {
        os << "null";
    } else if (const int64_t *i = val.get<int64_t>()) {
        os << *i;
    } else if (const double *d = val.get<double>()) {
        os << *d;
    } else if (const bool *b = val.get<bool>()) {
        if (*b)
            os << "true";
        else
            os << "false";
    } else if (const std::string *s = val.get<std::string>()) {
        json::escape_string_to_stream(os, *s);
    }
}

std::unordered_map<std::string, json::Type>::iterator JsonObject::begin() {
    return data.begin();
}
std::unordered_map<std::string, json::Type>::const_iterator JsonObject::begin() const {
    return data.begin();
}

std::unordered_map<std::string, json::Type>::iterator JsonObject::end() {
    return data.end();
}
std::unordered_map<std::string, json::Type>::const_iterator JsonObject::end() const {
    return data.end();
}

std::list<std::string>::const_iterator JsonObject::keys_begin() const {
    return keys.begin();
}

std::list<std::string>::const_iterator JsonObject::keys_end() const {
    return keys.end();
}

void JsonObject::to_pretty_stream(std::ostream &os, int indentations) const {
    os << '{';
    if (data.empty()) {
        os << '}';
        return;
    }
    os << '\n';

    for (auto it = keys.cbegin(); it != keys.cend(); ++it) {
        if (it != keys.begin())
            os << ",\n";
        for (int i = 0; i < indentations + 1; i++) {
            os << '\t';
        }
        os << "\"" << *it << "\" : ";
        const json::Type &val = data.at(*it);
        ::to_pretty_stream(os, val, indentations + 1);
    }
    os << '\n';
    for (int i = 0; i < indentations; i++)
        os << '\t';
    os << '}';
}

void JsonObject::to_stream(std::ostream &os) const {
    os << '{';
    for (auto it = keys.begin(); it != keys.end(); ++it) {
        if (it != keys.begin())
            os << ',';
        os << '"' << *it << "\":";
        const json::Type &val = data.at(*it);
        json::to_stream(os, val);
    }
    os << '}';
}

void JsonList::to_pretty_stream(std::ostream &os, int indentations) const {
    os << '[';
    if (data.empty()) {
        os << ']';
        return;
    }
    os << '\n';

    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it != data.begin())
            os << ",\n";
        for (int i = 0; i < indentations + 1; i++) {
            os << '\t';
        }
        const json::Type &val = *it;
        ::to_pretty_stream(os, val, indentations + 1);
    }
    os << '\n';
    for (int i = 0; i < indentations; i++)
        os << '\t';
    os << ']';
}

void JsonList::to_stream(std::ostream &os) const {
    os << '[';
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it != data.begin())
            os << ',';
        const json::Type &val = *it;
        json::to_stream(os, val);
    }
    os << ']';
}

std::ostream &operator<<(std::ostream &os, const JsonObject &obj) {
    obj.to_pretty_stream(os, 0);
    return os;
}

std::ostream &operator<<(std::ostream &os, const JsonList &list) {
    list.to_pretty_stream(os, 0);
    return os;
}

