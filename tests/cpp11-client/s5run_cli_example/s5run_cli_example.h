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

class File {
public:
    virtual ~File() {}
    virtual int64_t GetSize() = 0;
    virtual std::vector<uint8_t> ReadBytes(int64_t readBytes) = 0;
};

class FileSystem {
public:
    virtual ~FileSystem() {}
    virtual std::vector<std::string> GetList(const std::string & filepath) = 0;
};

class console {
public:
    virtual ~console() {}
    virtual void log(const std::vector<std::string> & args) = 0;
    virtual void warning(const std::vector<std::string> & args) = 0;
    virtual void error(const std::vector<std::string> & args) = 0;
};

class GlobalFunction_GetFile {
public:
    virtual ~GlobalFunction_GetFile() {}
    virtual std::shared_ptr<File> execute(const std::string & filepath) = 0;
};

class GlobalFunction_GetTimeNow {
public:
    virtual ~GlobalFunction_GetTimeNow() {}
    virtual int64_t execute() = 0;
};

class GlobalFunction_HasFile {
public:
    virtual ~GlobalFunction_HasFile() {}
    virtual bool execute(const std::string & filepath) = 0;
};

class GlobalFunction_GetFileList {
public:
    virtual ~GlobalFunction_GetFileList() {}
    virtual std::vector<std::string> execute(const std::string & filepath) = 0;
};

class client {
public:
    client();
    ~client();

    // Address format: s5run://host:port
    void set_server_address(const std::string &address);

    // Registers the object type on the server (class name + methods).
    // T must inherit from one of the generated base classes.
    template <typename T>
    void registry_class() {
        _registry_class_impl(static_cast<T *>(0));
    }

    // Registers a global function implementation.
    template <typename T>
    void registry_global_function() {
        _registry_global_function_impl(new T());
    }

    // Registers a singleton object, visible in scripts as `name()`.
    // The raw pointer overload takes ownership.
    void registry_global_object(File *obj);
    void registry_global_object(FileSystem *obj);
    void registry_global_object(console *obj);

    void registry_global_object(std::shared_ptr<File> obj);
    void registry_global_object(std::shared_ptr<FileSystem> obj);
    void registry_global_object(std::shared_ptr<console> obj);

    // Runs the script; serves callbacks while it executes.
    // Throws std::runtime_error on failure.
    value run(const std::string &script, const std::string &entry,
              const std::vector<value> &args = std::vector<value>());

private:
    client(const client &);
    client &operator=(const client &);

    void _registry_class_impl(File *);
    void _registry_class_impl(FileSystem *);
    void _registry_class_impl(console *);

    void _registry_global_function_impl(GlobalFunction_GetFile *);
    void _registry_global_function_impl(GlobalFunction_GetTimeNow *);
    void _registry_global_function_impl(GlobalFunction_HasFile *);
    void _registry_global_function_impl(GlobalFunction_GetFileList *);

    class impl;
    impl *m_impl;
};

} // namespace s5run_cli_example
