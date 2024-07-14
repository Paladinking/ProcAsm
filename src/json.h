#ifndef JSON_00_H
#define JSON_00_H
#include "engine/exceptions.h"
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * json_exception, for when reading a json file fails.
 */
class json_exception : public base_exception {
public:
    explicit json_exception(std::string msg) : base_exception(std::move(msg)){};
};

class JsonObject;

class JsonList;

namespace json {

struct Type {
    enum { OBJECT, LIST, INTEGER, DOUBLE, BOOL, STRING, JSON_NULL } type;

private:
    struct DummyObj {
        union {
            unsigned char data[104];
            uint64_t t;
        };
    };
    struct DummyList {
        union {
            unsigned char data[32];
            uint64_t t;
        };
    };

    static_assert(sizeof(DummyObj) == 104, "Must match JsonObject");
    static_assert(alignof(DummyObj) == 8, "Must match JsonObject");
    static_assert(sizeof(DummyList) == 32, "Must match JsonList");
    static_assert(alignof(DummyList) == 8, "Must match JsonList");
    union {
        DummyObj object;
        DummyList list;
        int64_t integer;
        double decimal;
        bool boolean;
        std::string str;
    };

    void free();

public:
    Type();
    Type(const Type &other);
    Type(Type &&other);
    Type &operator=(const Type &other);
    Type &operator=(Type &&other);
    ~Type();

    template <class T> T *get();

    template <class T> const T *get() const;

    template <class T> void set(T t);
};

/**
 * Reads a JsonObject from a string.
 */
JsonObject read_from_string(const std::string &in);

/**
 * Writes a JsonObject to a string, in prettified form with indentations and
 * newlines.
 */
std::string write_to_string(const JsonObject &obj);

/**
 * Writes a JsonObject to a string.
 * If pretty is true, indentations and newlines will be written.
 */
std::string write_to_string(const JsonObject &obj, bool pretty);

/**
 *  Writes a json::Type variant to a stream, in prettified form with
 * indentations and newlines.
 */
void to_pretty_stream(std::ostream &os, const Type &type);

/**
 *  Writes a json::Type variant to a stream, in compact form.
 */
void to_stream(std::ostream &os, const Type &type);

/**
 * Writes a string to a stream surrounded with quotes and replaces \, ", LF,
 * tab, backspace, form feed and CR with
 * \\, \", \n, \t, \b, \f  and \r respectively.
 */
void escape_string_to_stream(std::ostream &os, const std::string &s);
} // namespace json

/**
 * Class for representing a Json object.
 */
class JsonObject {
public:
    /**
     * Gets a value of type T with key key from the object.
     */
    template <class T> T &get(const std::string &key);
    template <class T> const T &get(const std::string &key);

    /**
     * Gets a value of type T with key key from the object.
     * If no such value exists, default_val is returned.
     */
    template <class T>
    const T &get_default(const std::string &key, const T &default_val) const;
    template <class T> T &get_default(const std::string &key, T &default_val);

    /**
     * Gets a json::Type with key key from the object.
     */
    const json::Type &get(const std::string &key) const { return data.at(key); }
    json::Type &get(const std::string &key) { return data.at(key); }

    /**
     * Sets the value at key to value.
     */
    template <class T> void set(const std::string &key, T value);

    /**
     * Gets a beginning iterator to the underlying map. No iteration order is
     * guaranteed.
     */
    std::unordered_map<std::string, json::Type>::iterator begin();
    std::unordered_map<std::string, json::Type>::const_iterator begin() const;

    /**
     * Gets an end iterator to the underlying map.
     */
    std::unordered_map<std::string, json::Type>::iterator end();
    std::unordered_map<std::string, json::Type>::const_iterator end() const;

    /**
     * Gets an beginning iterator to the keys of this object.
     * The order of iteration is the order of insertion.
     */
    std::list<std::string>::const_iterator keys_begin() const;

    /**
     * Gets an end iterator to the keys of this object.
     */
    std::list<std::string>::const_iterator keys_end() const;

    /**
     * Returns true if this object contains the key key.
     */
    bool has_key(const std::string &key) const { return data.count(key) != 0; }

    /**
     * Returns true if this object contains the key key,
     *	and the value at key has the type T.
     */
    template <class T> bool has_key_of_type(const std::string &key) const;

    /**
     * Returns the number of elements in this object.
     */
    size_t size() const { return data.size(); }

    /**
     * Removes all elements from this object.
     */
    void clear() {
        data.clear();
        keys.clear();
    }

    /**
     * Outputs this object as text to a stream, using indentations and spaces.
     * The keys will be ordered the same as the order of insertion.
     */
    void to_pretty_stream(std::ostream &os, int indentations) const;

    /**
     * Outputs this object as text to a stream.
     * The keys will be ordered the same as the order of insertion.
     */
    void to_stream(std::ostream &os) const;

private:
    std::unordered_map<std::string, json::Type> data;

    std::list<std::string> keys;
};
static_assert(sizeof(JsonObject) <= 104, "Change json::Type to match");
static_assert(alignof(JsonObject) == 8, "Change json::Type to match");

/**
 * Class representing a Json list.
 */
class JsonList {
public:
    /**
     * Returns true if this list has an entry at index with type T.
     */
    template <class T> bool has_index_of_type(const std::size_t index) const;

    /**
     * Gets the element of type T at index from this list.
     */
    template <class T> T &get(std::size_t ix);

    template <class T> const T &get(std::size_t ix) const;

    /**
     * Gets the json::Type at index from this list.
     */
    json::Type &get(const unsigned index) { return data[index]; }

    const json::Type &get(const unsigned index) const { return data[index]; }

    /**
     * Sets the entry at index to value.
     */
    template <class T> void set(std::size_t index, T value);

    /**
     * Append an element of type T to the end of the list.
     */
    template <class T> void push_back(T value);

    /**
     * Gets an iterator to the beginning of the list.
     */
    std::vector<json::Type>::iterator begin() { return data.begin(); }
    std::vector<json::Type>::const_iterator begin() const {
        return data.begin();
    }

    /**
     * Gets an iterator to the end of the list.
     */
    std::vector<json::Type>::iterator end() { return data.end(); }
    std::vector<json::Type>::const_iterator end() const { return data.end(); }

    /**
     * Returns the number of entries in the list.
     */
    size_t size() const { return data.size(); }

    /**
     * Removes all entries from the list.
     */
    void clear() { data.clear(); }

    /**
     * Outputs this list as a string to a stream, using indentations and spaces.
     */
    void to_pretty_stream(std::ostream &os, int indentations) const;

    /**
     * Outputs this list as a string to a stream.
     */
    void to_stream(std::ostream &os) const;

private:
    std::vector<json::Type> data;
};

static_assert(sizeof(JsonList) <= 32, "Change json::Type to match");
static_assert(alignof(JsonList) == 8, "Change json::Type to match");

namespace json {

template <> const JsonObject *Type::get() const;
template <> const JsonList *Type::get() const;
template <> const std::string *Type::get() const;
template <> const int64_t *Type::get() const;
template <> const double *Type::get() const;
template <> const bool *Type::get() const;

template <> void Type::set(JsonObject obj);
template <> void Type::set(JsonList list);
template <> void Type::set(std::string s);
template <> void Type::set(int64_t i);
template <> void Type::set(double d);
template <> void Type::set(bool b);
template <> void Type::set(std::nullptr_t n);

template <class T> void Type::set(T t) {
    static_assert(std::is_same_v<T, JsonObject>, "Not a valid json type");
}

template <class T> const T *Type::get() const {
    static_assert(std::is_same_v<T, JsonObject>, "Not a valid json type");
}

template <class T> T *Type::get() {
    return const_cast<T *>(const_cast<const Type *>(this)->get<T>());
}

} // namespace json

template <class T> T &JsonObject::get(const std::string &key) {
    return *data[key].get<T>();
}
template <class T> const T &JsonObject::get(const std::string &key) {
    return *data[key].get<T>();
}

template <class T>
const T &JsonObject::get_default(const std::string &key,
                                 const T &default_val) const {
    const auto el = data.find(key);
    if (el == data.end())
        return default_val;
    const T *v = el->second.get<T>();
    if (v == nullptr) {
        return default_val;
    }
    return *v;
}

template <class T>
T &JsonObject::get_default(const std::string &key, T &default_val) {
    auto el = data.find(key);
    if (el == data.end())
        return default_val;
    T *v = el->second.get<T>();
    if (v == nullptr) {
        return default_val;
    }
    return *v;
}

template <class T> void JsonObject::set(const std::string &key, T val) {
    auto el = data.find(key);
    if (el == data.end()) {
        keys.push_back(key);
        data[key].set<T>(std::move(val));
    } else {
        el->second.set<T>(std::move(val));
    }
}

template <class T>
bool JsonObject::has_key_of_type(const std::string &key) const {
    auto el = data.find(key);
    if (el == data.end()) {
        return false;
    }
    return el->second.get<T>() != nullptr;
}

template <class T> bool JsonList::has_index_of_type(std::size_t ix) const {
    if (ix >= data.size()) {
        return false;
    }
    return data[ix].get<T>() != nullptr;
}

template <class T> T &JsonList::get(std::size_t ix) {
    return *data[ix].get<T>();
}

template <class T> const T &JsonList::get(std::size_t ix) const {
    return *data[ix].get<T>();
}

template <class T> void JsonList::set(std::size_t ix, T val) {
    data[ix].set<T>(std::move(val));
}

template <class T> void JsonList::push_back(T val) {
    data.emplace_back();
    data[data.size() - 1].set<T>(std::move(val));
}

/**
 * Calls to_pretty_stream on obj.
 */
std::ostream &operator<<(std::ostream &os, const JsonObject &obj);

/**
 * Calls to_pretty_stream on list.
 */
std::ostream &operator<<(std::ostream &os, const JsonList &list);

#endif
