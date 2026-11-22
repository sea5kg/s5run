// AUTO-GENERATED. DO NOT EDIT.
// C++98 legacy client, portable across Windows (WinSock2) and Linux.
#ifndef S5RUN_CLI_CPP98_H
#define S5RUN_CLI_CPP98_H

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

namespace s5run_cli_cpp98 {

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

class value {
public:
    enum kind { K_NULL, K_BOOL, K_INT, K_DOUBLE, K_STRING, K_ARRAY, K_OBJECT, K_REF };
    value(); value(bool b); value(i64 i); value(double d); value(const char *s);
    value(const std::string &s); value(const std::vector<value> &a);
    value(const std::map<std::string, value> &o);
    value(const value &other); value &operator=(const value &other); ~value();
    static value make_ref(int this_id, const std::string &type);
    kind get_kind() const; bool is_null() const; bool as_bool() const;
    i64 as_int() const; double as_double() const;
    const std::string &as_string() const;
    const std::vector<value> &as_array() const;
    const std::map<std::string, value> &as_object() const;
    int ref_this_id() const; const std::string &ref_type() const;
private:
    void destroy(); void copy_from(const value &other);
    kind m_kind; bool m_b; i64 m_i; double m_d;
    std::string m_s; std::vector<value> *m_a; std::map<std::string, value> *m_o;
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

class s5run_object {
public:
    virtual ~s5run_object() {}
    virtual std::string s5run_class_name() const = 0;
    virtual std::vector<std::string> s5run_method_names() const = 0;
};

class s5run_function {
public:
    virtual ~s5run_function() {}
    virtual std::string s5run_function_name() const = 0;
};

class base_context_class_File : public s5run_object {
public:
    virtual ~base_context_class_File() {}
    virtual std::string s5run_class_name() const { return "File"; }
    virtual std::vector<std::string> s5run_method_names() const {
        std::vector<std::string> out;
        out.push_back("GetSize");
        out.push_back("ReadBytes");
        out.push_back("GetName");
        out.push_back("GetBufferSize");
        out.push_back("SetBufferSize");
        return out;
    }
    virtual i64 GetSize() = 0;
    virtual std::vector<u8> ReadBytes(i64 readBytes) = 0;
    virtual std::string GetName() = 0;
    virtual i64 GetBufferSize() = 0;
    virtual void SetBufferSize(i64 bufferSize) = 0;
};

class base_context_class_FileSystem : public s5run_object {
public:
    virtual ~base_context_class_FileSystem() {}
    virtual std::string s5run_class_name() const { return "FileSystem"; }
    virtual std::vector<std::string> s5run_method_names() const {
        std::vector<std::string> out;
        out.push_back("GetList");
        return out;
    }
    virtual std::vector<std::string> GetList(const std::string & filepath) = 0;
};

class base_context_class_console : public s5run_object {
public:
    virtual ~base_context_class_console() {}
    virtual std::string s5run_class_name() const { return "console"; }
    virtual std::vector<std::string> s5run_method_names() const {
        std::vector<std::string> out;
        out.push_back("log");
        out.push_back("warning");
        out.push_back("error");
        return out;
    }
    virtual void log(const std::vector<std::string> & args) = 0;
    virtual void warning(const std::vector<std::string> & args) = 0;
    virtual void error(const std::vector<std::string> & args) = 0;
};

class base_context_function_GetFile : public s5run_function {
public:
    virtual std::string s5run_function_name() const { return "GetFile"; }
    virtual ~base_context_function_GetFile() {}
    virtual base_context_class_File * execute(const std::string & filepath) = 0;
};

class base_context_function_GetFileSize : public s5run_function {
public:
    virtual std::string s5run_function_name() const { return "GetFileSize"; }
    virtual ~base_context_function_GetFileSize() {}
    virtual i64 execute(base_context_class_File * file) = 0;
};

class base_context_function_GetTimeNow : public s5run_function {
public:
    virtual std::string s5run_function_name() const { return "GetTimeNow"; }
    virtual ~base_context_function_GetTimeNow() {}
    virtual i64 execute() = 0;
};

class base_context_function_HasFile : public s5run_function {
public:
    virtual std::string s5run_function_name() const { return "HasFile"; }
    virtual ~base_context_function_HasFile() {}
    virtual bool execute(const std::string & filepath) = 0;
};

class base_context_function_GetFileList : public s5run_function {
public:
    virtual std::string s5run_function_name() const { return "GetFileList"; }
    virtual ~base_context_function_GetFileList() {}
    virtual std::vector<std::string> execute(const std::string & filepath) = 0;
};

class base_context_function_GetOpenedFiles : public s5run_function {
public:
    virtual std::string s5run_function_name() const { return "GetOpenedFiles"; }
    virtual ~base_context_function_GetOpenedFiles() {}
    virtual std::vector<base_context_class_File *> execute() = 0;
};

class base_context_function_CloseFiles : public s5run_function {
public:
    virtual std::string s5run_function_name() const { return "CloseFiles"; }
    virtual ~base_context_function_CloseFiles() {}
    virtual void execute(const std::vector<base_context_class_File *> & files) = 0;
};

class base_context_function_GetPi : public s5run_function {
public:
    virtual std::string s5run_function_name() const { return "GetPi"; }
    virtual ~base_context_function_GetPi() {}
    virtual double execute() = 0;
};

class base_context_function_GetServerPort : public s5run_function {
public:
    virtual std::string s5run_function_name() const { return "GetServerPort"; }
    virtual ~base_context_function_GetServerPort() {}
    virtual i64 execute() = 0;
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
                                 const std::vector<value> &args);
    context_impl *m_impl;
};

class run_result {
public:
    run_result();
    bool has_error() const; const std::string &error() const;
    int error_line() const; i64 duration() const; const value &get_value() const;
private:
    friend class context::context_impl;
    friend run_result run_script(context *ctx, const std::string &entry,
                                 const std::vector<value> &args);
    bool m_has_error; std::string m_error; int m_error_line; i64 m_duration_ms;
    value m_value;
};

run_result run_script(context *ctx, const std::string &entry, const std::vector<value> &args = std::vector<value>());

} // namespace s5run_cli_cpp98

#endif // S5RUN_CLI_CPP98_H
