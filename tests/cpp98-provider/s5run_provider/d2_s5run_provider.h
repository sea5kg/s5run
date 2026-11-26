// AUTO-GENERATED. DO NOT EDIT.
// C++98 legacy client, portable across Windows (WinSock2) and Linux.
#ifndef D2_S5RUN_PROVIDER_H
#define D2_S5RUN_PROVIDER_H

#include <map>
#include <string>
#include <vector>

// Prevent Win32 API macros (windows.h) from renaming schema methods
// (e.g. #define SetProp SetPropW breaks method declarations/calls).
#undef SetProp
#undef GetProp
#undef RemoveProp
#undef CreateFile
#undef DeleteFile
#undef CopyFile
#undef MoveFile
#undef GetObject
#undef LoadLibrary
#undef FreeLibrary
#undef DrawText
#undef LoadIcon
#undef LoadCursor
#undef MessageBox
#undef SendMessage
#undef PostMessage
#undef SetFocus
#undef GetFocus
#undef ShowWindow
#undef UpdateWindow
#undef FindWindow
#undef GetTickCount

namespace s5run_provider {

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#ifdef _MSC_VER
typedef __int64 i64;
typedef unsigned __int64 u64;
#else
typedef long long i64;
typedef unsigned long long u64;
#endif
typedef signed char i8;
typedef short i16;
typedef int i32;

class context;
class run_result;
context * make_context();

class s5run_variable {
public:
    enum kind { K_NULL, K_BOOL, K_INT, K_DOUBLE, K_STRING, K_ARRAY, K_OBJECT, K_REF };
    s5run_variable(); s5run_variable(bool b); s5run_variable(i64 i); s5run_variable(double d); s5run_variable(const char *s);
    s5run_variable(const std::string &s); s5run_variable(const std::vector<s5run_variable> &a);
    s5run_variable(const std::map<std::string, s5run_variable> &o);
    s5run_variable(const s5run_variable &other); s5run_variable &operator=(const s5run_variable &other); ~s5run_variable();
    static s5run_variable make_ref(int this_id, const std::string &type);
    // Structured multi-line representation, handy for tests:
    //   K_INT 1024
    //   K_STRING "text\nline"
    //   K_OBJECT
    //   BEGIN
    //     size K_INT 1024
    //   END
    // Object fields are sorted by name.
    std::string to_string_s5run() const;
    // JSON representation (hand-rolled, no external libraries).
    std::string to_string_json() const;
    kind get_kind() const; bool is_null() const; bool as_bool() const;
    i64 as_int() const; double as_double() const;
    const std::string &as_string() const;
    const std::vector<s5run_variable> &as_array() const;
    const std::map<std::string, s5run_variable> &as_object() const;
    int ref_this_id() const; const std::string &ref_type() const;
private:
    void destroy(); void copy_from(const s5run_variable &other);
    void append_debug(std::string &out, int indent) const;
    static void append_json_string(std::string &out, const std::string &s);
    void append_json(std::string &out) const;
    kind m_kind; bool m_b; i64 m_i; double m_d;
    std::string m_s; std::vector<s5run_variable> *m_a; std::map<std::string, s5run_variable> *m_o;
};

class remote_server_info {
public:
    remote_server_info();
    void set_address(const std::string &address);
    const std::string &get_address() const;
    void set_api_key(const std::string &api_key);
    const std::string &get_api_key() const;
    const std::string &get_host() const; int get_port() const;
private:
    std::string m_address; std::string m_api_key; std::string m_host; int m_port;
};

struct s5run_property_descriptor {
    std::string name;
    std::string getter;
    std::string setter;
    s5run_property_descriptor() {}
    s5run_property_descriptor(const std::string &n, const std::string &g, const std::string &s)
        : name(n), getter(g), setter(s) {}
};

struct s5run_arg_metadata {
    std::string name;
    std::string type;
    s5run_arg_metadata() {}
    s5run_arg_metadata(const std::string &n, const std::string &t) : name(n), type(t) {}
};

struct s5run_method_metadata {
    std::string name;
    std::string return_type;
    std::vector<s5run_arg_metadata> args;
};

struct s5run_class_metadata {
    std::string name;
    std::vector<s5run_method_metadata> methods;
    std::vector<s5run_property_descriptor> properties;
};

struct s5run_function_metadata {
    std::string name;
    std::string return_type;
    std::vector<s5run_arg_metadata> args;
};

class s5run_object {
public:
    virtual ~s5run_object() {}
    virtual std::string s5run_class_name() const = 0;
    virtual std::vector<std::string> s5run_method_names() const = 0;
    // property descriptors {name, getter, setter}; JS: obj.<name> read/write
    virtual std::vector<s5run_property_descriptor> s5run_property_descriptors() const {
        return std::vector<s5run_property_descriptor>();
    }
    // full class metadata (NULL for custom classes without a base)
    virtual const s5run_class_metadata *s5run_metadata() const { return 0; }
};

class s5run_function {
public:
    virtual ~s5run_function() {}
    virtual std::string s5run_function_name() const = 0;
    // full function metadata (NULL for custom functions without a base)
    virtual const s5run_function_metadata *s5run_metadata() const { return 0; }
};

class base_context_class_MassageCrew_Queue : public s5run_object {
public:
    virtual ~base_context_class_MassageCrew_Queue() {}
    virtual std::string s5run_class_name() const { return "MassageCrew_Queue"; }
    virtual std::vector<s5run_property_descriptor> s5run_property_descriptors() const {
        std::vector<s5run_property_descriptor> out;
        return out;
    }
    virtual const s5run_class_metadata *s5run_metadata() const;
    static const s5run_class_metadata &metadata();
    virtual std::vector<std::string> s5run_method_names() const {
        std::vector<std::string> out;
        out.push_back("publish");
        return out;
    }
    virtual void publish(const std::string & msg_id, const std::string & msg_text) = 0;
};

class base_context_function_getQueue : public s5run_function {
public:
    virtual std::string s5run_function_name() const { return "getQueue"; }
    virtual ~base_context_function_getQueue() {}
    virtual const s5run_function_metadata *s5run_metadata() const;
    static const s5run_function_metadata &metadata();
    virtual base_context_class_MassageCrew_Queue * execute(const std::string & queue_name) = 0;
};

class context {
public:
    context(); ~context();
    static const char *client_version() { return "1.0.0"; }
    void set_script(const std::string &script);
    void add_server(remote_server_info *server);
    void add_class(s5run_object *sample);
    void add_object(s5run_object *obj);
    void add_function(s5run_function *fn);
    class context_impl;
private:
    friend run_result run_script(context *ctx, const std::string &entry,
                                 const std::vector<s5run_variable> &args);
    friend bool serve(context *ctx, const std::string &host, int port);
    context_impl *m_impl;
};

class run_result {
public:
    run_result();
    bool has_error() const; const std::string &error() const;
    int error_line() const; i64 duration() const; const s5run_variable &get_value() const;
private:
    friend class context::context_impl;
    friend run_result run_script(context *ctx, const std::string &entry,
                                 const std::vector<s5run_variable> &args);
    bool m_has_error; std::string m_error; int m_error_line; i64 m_duration_ms;
    s5run_variable m_value;
};

run_result run_script(context *ctx, const std::string &entry, const std::vector<s5run_variable> &args = std::vector<s5run_variable>());

// Provider mode: connect, register the context and serve incoming
// callback requests until the connection closes.
// Returns false on a disconnect or an error.
bool serve(context *ctx, const std::string &host, int port);

} // namespace s5run_provider

#endif // D2_S5RUN_PROVIDER_H
