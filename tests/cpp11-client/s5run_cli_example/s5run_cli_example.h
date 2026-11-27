// AUTO-GENERATED. DO NOT EDIT.
#pragma once

#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace s5run_cli_example {

const int S5RUN_PROTOCOL_VERSION = 3;

class context;
class s5run_remote_server_info;
class run_result;
std::shared_ptr<context> make_context();

class s5run_variable {
public:
    enum kind {
        K_UNDEFINED, K_NULL, K_BOOL, K_INT, K_DOUBLE, K_STRING, K_ARRAY, K_OBJECT, K_REF
    };

    s5run_variable() : m_kind(K_UNDEFINED), m_b(false), m_i(0), m_d(0.0) {}
    s5run_variable(bool b) : m_kind(K_BOOL), m_b(b), m_i(0), m_d(0.0) {}
    s5run_variable(int i) : m_kind(K_INT), m_b(false), m_i(i), m_d(0.0) {}
    s5run_variable(int64_t i) : m_kind(K_INT), m_b(false), m_i(i), m_d(0.0) {}
    s5run_variable(double d) : m_kind(K_DOUBLE), m_b(false), m_i(0), m_d(d) {}
    s5run_variable(const char *s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s) {}
    s5run_variable(const std::string &s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s) {}
    s5run_variable(const std::vector<s5run_variable> &a) : m_kind(K_ARRAY), m_b(false), m_i(0), m_d(0.0), m_a(a) {}
    s5run_variable(const std::map<std::string, s5run_variable> &o) : m_kind(K_OBJECT), m_b(false), m_i(0), m_d(0.0), m_o(o) {}

    static s5run_variable make_ref(int this_id, const std::string &type) {
        s5run_variable v;
        v.m_kind = K_REF;
        v.m_i = this_id;
        v.m_s = type;
        return v;
    }

    kind get_kind() const { return m_kind; }
    bool is_undefined() const { return m_kind == K_UNDEFINED; }
    bool is_null() const { return m_kind == K_NULL; }

    bool as_bool() const { return m_b; }
    int64_t as_int() const { return m_i; }
    double as_double() const { return m_d; }
    const std::string &as_string() const { return m_s; }
    const std::vector<s5run_variable> &as_array() const { return m_a; }
    const std::map<std::string, s5run_variable> &as_object() const { return m_o; }
    int ref_this_id() const { return (int)m_i; }
    const std::string &ref_type() const { return m_s; }

    // Structured representation, handy for tests and debugging, e.g.
    //   K_INT 1024
    //   K_STRING "text\nline"
    //   K_OBJECT
    //   BEGIN
    //     size K_INT 1024
    //   END
    // Object fields are sorted by name.
    std::string to_string_s5run() const {
        std::string out;
        append_debug(out, 0);
        if (!out.empty() && out[out.size() - 1] == '\n')
            out.erase(out.size() - 1);
        return out;
    }

    // JSON representation (hand-rolled, no external libraries), e.g.
    // {"size": 1024, "tags": ["a", "b"], "ok": true}
    std::string to_string_json() const {
        std::string out;
        append_json(out);
        return out;
    }

private:
    // Structured multi-line form: the first line is emitted without an
    // indent prefix (the caller positions it - column 0, after an object
    // key or an array element prefix), continuation lines carry the
    // given indent.
    void append_debug(std::string &out, int indent) const {
        char buf[64];
        std::string pad(indent, ' ');
        std::string pad_in(indent + 2, ' ');
        switch (m_kind) {
        case K_UNDEFINED:
            out += "K_UNDEFINED\n";
            break;
        case K_NULL:
            out += "K_NULL\n";
            break;
        case K_BOOL:
            out += m_b ? "K_BOOL true\n" : "K_BOOL false\n";
            break;
        case K_INT:
            snprintf(buf, sizeof(buf), "K_INT %lld\n", (long long)m_i);
            out += buf;
            break;
        case K_DOUBLE:
            snprintf(buf, sizeof(buf), "K_DOUBLE %g\n", m_d);
            out += buf;
            break;
        case K_STRING:
            out += "K_STRING ";
            append_json_string(out, m_s);
            out += "\n";
            break;
        case K_ARRAY:
            out += "K_ARRAY\n";
            out += pad + "BEGIN\n";
            for (size_t i = 0; i < m_a.size(); ++i) {
                out += pad_in;
                m_a[i].append_debug(out, indent + 2);
            }
            out += pad + "END\n";
            break;
        case K_OBJECT:
            // std::map iterates sorted by name
            out += "K_OBJECT\n";
            out += pad + "BEGIN\n";
            for (auto it = m_o.begin(); it != m_o.end(); ++it) {
                out += pad_in + it->first + " ";
                it->second.append_debug(out, indent + 2);
            }
            out += pad + "END\n";
            break;
        case K_REF:
            out += "K_REF ";
            out += std::to_string(m_i);
            out += " ";
            append_json_string(out, m_s);
            out += "\n";
            break;
        }
    }

    static void append_json_string(std::string &out, const std::string &s) {
        out += '"';
        for (size_t i = 0; i < s.size(); ++i) {
            char c = s[i];
            switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if ((unsigned char)c < 0x20) {
                    char b[8];
                    snprintf(b, sizeof(b), "\\u%04x", (unsigned char)c);
                    out += b;
                } else {
                    out += c;
                }
            }
        }
        out += '"';
    }

    void append_json(std::string &out) const {
        char buf[64];
        switch (m_kind) {
        case K_UNDEFINED:
        case K_NULL: out += "null"; break;
        case K_BOOL: out += m_b ? "true" : "false"; break;
        case K_INT:
            snprintf(buf, sizeof(buf), "%lld", (long long)m_i);
            out += buf;
            break;
        case K_DOUBLE:
            snprintf(buf, sizeof(buf), "%g", m_d);
            out += buf;
            break;
        case K_STRING:
            append_json_string(out, m_s);
            break;
        case K_ARRAY: {
            out += "[";
            for (size_t i = 0; i < m_a.size(); ++i) {
                if (i > 0)
                    out += ", ";
                m_a[i].append_json(out);
            }
            out += "]";
            break;
        }
        case K_OBJECT: {
            out += "{";
            bool first = true;
            for (auto it = m_o.begin(); it != m_o.end(); ++it) {
                if (!first)
                    out += ", ";
                first = false;
                append_json_string(out, it->first);
                out += ": ";
                it->second.append_json(out);
            }
            out += "}";
            break;
        }
        case K_REF:
            out += "{\"__s5run_ref__\":{\"this_id\":";
            out += std::to_string(m_i);
            out += ",\"type\":";
            append_json_string(out, m_s);
            out += "}}";
            break;
        }
    }

    kind m_kind;
    bool m_b;
    int64_t m_i;
    double m_d;
    std::string m_s;
    std::vector<s5run_variable> m_a;
    std::map<std::string, s5run_variable> m_o;
};
class s5run_remote_server_info {
public:
    s5run_remote_server_info() : m_port(0), m_have_address(false) {}

    // Address format: s5run://host:port
    void set_address(const std::string &address);
    const std::string &get_address() const { return m_address; }

    void set_api_key(const std::string &api_key) { m_api_key = api_key; }
    const std::string &get_api_key() const { return m_api_key; }

    // Valid after a successful set_address().
    const std::string &get_host() const { return m_host; }
    uint16_t get_port() const { return m_port; }

private:
    std::string m_address;
    std::string m_api_key;
    std::string m_host;
    uint16_t m_port;
    bool m_have_address;
};
struct s5run_property_descriptor {
    std::string name;
    std::string getter;
    std::string setter;
    s5run_property_descriptor() {}
    s5run_property_descriptor(const std::string &n, const std::string &g, const std::string &s)
        : name(n), getter(g), setter(s) {}
};

class s5run_object {
public:
    virtual ~s5run_object() {}
    virtual std::string s5run_class_name() const = 0;
    virtual std::vector<std::string> s5run_method_names() const = 0;
    // property descriptors {name, getter, setter}; JS: obj.<name> read/write
    virtual std::vector<s5run_property_descriptor> s5run_property_descriptors() const {
        return {};
    }
};

class File : public s5run_object {
public:
    std::string s5run_class_name() const override { return "File"; }
    std::vector<s5run_property_descriptor> s5run_property_descriptors() const override {
        return {s5run_property_descriptor("Name", "GetName", ""), s5run_property_descriptor("BufferSize", "GetBufferSize", "SetBufferSize")};
    }
    std::vector<std::string> s5run_method_names() const override {
        std::vector<std::string> out_;
        out_.push_back("GetSize");
        out_.push_back("ReadBytes");
        out_.push_back("GetName");
        out_.push_back("GetBufferSize");
        out_.push_back("SetBufferSize");
        return out_;
    }
    virtual ~File() {}
    virtual int64_t GetSize() = 0;
    virtual std::vector<uint8_t> ReadBytes(int64_t readBytes) = 0;
    virtual std::string GetName() = 0;
    virtual int64_t GetBufferSize() = 0;
    virtual void SetBufferSize(int64_t bufferSize) = 0;
};

class FileSystem : public s5run_object {
public:
    std::string s5run_class_name() const override { return "FileSystem"; }
    std::vector<s5run_property_descriptor> s5run_property_descriptors() const override {
        return {};
    }
    std::vector<std::string> s5run_method_names() const override {
        std::vector<std::string> out_;
        out_.push_back("GetList");
        return out_;
    }
    virtual ~FileSystem() {}
    virtual std::vector<std::string> GetList(const std::string & filepath) = 0;
};

class console : public s5run_object {
public:
    std::string s5run_class_name() const override { return "console"; }
    std::vector<s5run_property_descriptor> s5run_property_descriptors() const override {
        return {};
    }
    std::vector<std::string> s5run_method_names() const override {
        std::vector<std::string> out_;
        out_.push_back("log");
        out_.push_back("warning");
        out_.push_back("error");
        return out_;
    }
    virtual ~console() {}
    virtual void log(const std::vector<std::string> & args) = 0;
    virtual void warning(const std::vector<std::string> & args) = 0;
    virtual void error(const std::vector<std::string> & args) = 0;
};

class KeyValueStorage : public s5run_object {
public:
    std::string s5run_class_name() const override { return "KeyValueStorage"; }
    std::vector<s5run_property_descriptor> s5run_property_descriptors() const override {
        return {};
    }
    std::vector<std::string> s5run_method_names() const override {
        std::vector<std::string> out_;
        out_.push_back("SetValueByKey");
        out_.push_back("GetValueByKey");
        return out_;
    }
    virtual ~KeyValueStorage() {}
    virtual void SetValueByKey(const std::string & name, const s5run_variable & value) = 0;
    virtual void GetValueByKey(const std::string & name) = 0;
};

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

class GlobalFunction_GetFileSize : public s5run_function {
public:
    std::string s5run_function_name() const override { return "GetFileSize"; }
    virtual ~GlobalFunction_GetFileSize() {}
    virtual int64_t execute(const std::shared_ptr<File> & file) = 0;
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

class GlobalFunction_GetOpenedFiles : public s5run_function {
public:
    std::string s5run_function_name() const override { return "GetOpenedFiles"; }
    virtual ~GlobalFunction_GetOpenedFiles() {}
    virtual std::vector<std::shared_ptr<File>> execute() = 0;
};

class GlobalFunction_CloseFiles : public s5run_function {
public:
    std::string s5run_function_name() const override { return "CloseFiles"; }
    virtual ~GlobalFunction_CloseFiles() {}
    virtual void execute(const std::vector<std::shared_ptr<File>> & files) = 0;
};

class GlobalFunction_GetPi : public s5run_function {
public:
    std::string s5run_function_name() const override { return "GetPi"; }
    virtual ~GlobalFunction_GetPi() {}
    virtual double execute() = 0;
};

class GlobalFunction_GetServerPort : public s5run_function {
public:
    std::string s5run_function_name() const override { return "GetServerPort"; }
    virtual ~GlobalFunction_GetServerPort() {}
    virtual int64_t execute() = 0;
};

class context {
public:
    context();
    ~context();

    // Version of the generated client code.
    static const char *client_version() { return "1.0.0"; }

    // Address format: s5run://host:port
    // Adds a server the context talks to. The first added server
    // is used for script execution; the api_key is reserved for
    // future authentication.
    void add_server(const std::shared_ptr<s5run_remote_server_info> &server);

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

    // Sets the script executed by run_script().
    void set_script(const std::string &script);

    class impl; // public forward declaration: run_result befriends it

private:
    context(const context &);
    context &operator=(const context &);
    std::shared_ptr<run_result> _run(const std::string &entry, const std::vector<s5run_variable> &args);
    friend std::shared_ptr<run_result> run_script(const std::shared_ptr<context> &,
                                                const std::string &, const std::vector<s5run_variable> &);
    impl *m_impl;
};

class run_result {
public:
    bool has_error() const { return m_has_error; }
    const std::string &error() const { return m_error; }
    int error_line() const { return m_error_line; }
    int64_t duration() const { return m_duration_ms; } // milliseconds
    const s5run_variable &get_value() const { return m_value; }

private:
    friend class context::impl;
    run_result() : m_has_error(false), m_error_line(0), m_duration_ms(0) {}

    bool m_has_error;
    std::string m_error;
    int m_error_line;
    int64_t m_duration_ms;
    s5run_variable m_value;
};

// Runs the context script; serves callbacks while it executes.
// Never throws: all errors are reported via run_result.
inline std::shared_ptr<run_result> run_script(
    const std::shared_ptr<context> &ctx,
    const std::string &entry,
    const std::vector<s5run_variable> &args = std::vector<s5run_variable>()) {
    return ctx->_run(entry, args);
}

} // namespace s5run_cli_example
