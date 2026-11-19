// AUTO-GENERATED. DO NOT EDIT.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace s5run_cli_example {

class client;
std::shared_ptr<client> make_client();

// Generic value: used for run() arguments and results.
class value {
public:
    enum kind {
        K_NULL, K_BOOL, K_INT, K_DOUBLE, K_STRING, K_ARRAY, K_OBJECT, K_REF
    };

    value() : m_kind(K_NULL), m_b(false), m_i(0), m_d(0.0) {}
    value(bool b) : m_kind(K_BOOL), m_b(b), m_i(0), m_d(0.0) {}
    value(int i) : m_kind(K_INT), m_b(false), m_i(i), m_d(0.0) {}
    value(int64_t i) : m_kind(K_INT), m_b(false), m_i(i), m_d(0.0) {}
    value(double d) : m_kind(K_DOUBLE), m_b(false), m_i(0), m_d(d) {}
    value(const char *s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s) {}
    value(const std::string &s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s) {}
    value(const std::vector<value> &a) : m_kind(K_ARRAY), m_b(false), m_i(0), m_d(0.0), m_a(a) {}
    value(const std::map<std::string, value> &o) : m_kind(K_OBJECT), m_b(false), m_i(0), m_d(0.0), m_o(o) {}

    static value make_ref(int this_id, const std::string &type) {
        value v;
        v.m_kind = K_REF;
        v.m_i = this_id;
        v.m_s = type;
        return v;
    }

    kind get_kind() const { return m_kind; }
    bool is_null() const { return m_kind == K_NULL; }

    bool as_bool() const { return m_b; }
    int64_t as_int() const { return m_i; }
    double as_double() const { return m_d; }
    const std::string &as_string() const { return m_s; }
    const std::vector<value> &as_array() const { return m_a; }
    const std::map<std::string, value> &as_object() const { return m_o; }
    int ref_this_id() const { return (int)m_i; }
    const std::string &ref_type() const { return m_s; }

private:
    kind m_kind;
    bool m_b;
    int64_t m_i;
    double m_d;
    std::string m_s;
    std::vector<value> m_a;
    std::map<std::string, value> m_o;
};

// Common root of all generated classes: lets the client work with
// objects without knowing their concrete types.
class s5run_object {
public:
    virtual ~s5run_object() {}
    virtual std::string s5run_class_name() const = 0;
    virtual std::vector<std::string> s5run_method_names() const = 0;
};

class File : public s5run_object {
public:
    std::string s5run_class_name() const override { return "File"; }
    std::vector<std::string> s5run_method_names() const override {
        std::vector<std::string> out;
        out.push_back("GetSize");
        out.push_back("ReadBytes");
        return out;
    }
    virtual ~File() {}
    virtual int64_t GetSize() = 0;
    virtual std::vector<uint8_t> ReadBytes(int64_t readBytes) = 0;
};

class FileSystem : public s5run_object {
public:
    std::string s5run_class_name() const override { return "FileSystem"; }
    std::vector<std::string> s5run_method_names() const override {
        std::vector<std::string> out;
        out.push_back("GetList");
        return out;
    }
    virtual ~FileSystem() {}
    virtual std::vector<std::string> GetList(const std::string & filepath) = 0;
};

class console : public s5run_object {
public:
    std::string s5run_class_name() const override { return "console"; }
    std::vector<std::string> s5run_method_names() const override {
        std::vector<std::string> out;
        out.push_back("log");
        out.push_back("warning");
        out.push_back("error");
        return out;
    }
    virtual ~console() {}
    virtual void log(const std::vector<std::string> & args) = 0;
    virtual void warning(const std::vector<std::string> & args) = 0;
    virtual void error(const std::vector<std::string> & args) = 0;
};

// Common root of all generated global functions: lets the client
// register implementations without knowing their concrete types.
class s5run_function {
public:
    virtual ~s5run_function() {}
    virtual std::string s5run_function_name() const = 0;
};

class GlobalFunction_GetFile : public s5run_function {
public:
    std::string s5run_function_name() const override { return "GetFile"; }
    virtual ~GlobalFunction_GetFile() {}
    virtual std::shared_ptr<File> execute(const std::string & filepath) = 0;
};

class GlobalFunction_GetTimeNow : public s5run_function {
public:
    std::string s5run_function_name() const override { return "GetTimeNow"; }
    virtual ~GlobalFunction_GetTimeNow() {}
    virtual int64_t execute() = 0;
};

class GlobalFunction_HasFile : public s5run_function {
public:
    std::string s5run_function_name() const override { return "HasFile"; }
    virtual ~GlobalFunction_HasFile() {}
    virtual bool execute(const std::string & filepath) = 0;
};

class GlobalFunction_GetFileList : public s5run_function {
public:
    std::string s5run_function_name() const override { return "GetFileList"; }
    virtual ~GlobalFunction_GetFileList() {}
    virtual std::vector<std::string> execute(const std::string & filepath) = 0;
};

class client {
public:
    client();
    ~client();

    // Address format: s5run://host:port
    void set_server_address(const std::string &address);

    // Registers a remote context provider: the listed context
    // names (global objects / classes) will be supplied by the
    // s5run server at the given address. Stub, not implemented yet.
    void add_context_provider(const std::string &address,
                              const std::string &api_key,
                              const std::vector<std::string> &contexts);

    // Registers the object type on the server (class name + methods).
    // Pass any instance of a generated base class; only its type
    // information is used, the instance itself is not stored.
    void add_class(const std::shared_ptr<s5run_object> &sample);

    // Registers a singleton object, visible in scripts as
    // `name.method(...)`. The raw pointer overload takes ownership.
    void add_object(s5run_object *obj);
    void add_object(const std::shared_ptr<s5run_object> &obj);

    // Registers a global function implementation.
    void add_function(const std::shared_ptr<s5run_function> &fn);

    // Runs the script; serves callbacks while it executes.
    // Throws std::runtime_error on failure.
    value run(const std::string &script, const std::string &entry,
              const std::vector<value> &args = std::vector<value>());

private:
    client(const client &);
    client &operator=(const client &);


    class impl;
    impl *m_impl;
};

} // namespace s5run_cli_example
