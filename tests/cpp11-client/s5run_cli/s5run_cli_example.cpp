#include "run_script_client.h"
#include "s5run_protocol.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <map>
#include <stdexcept>

namespace example {

namespace {

// Convert Value <-> json::Value for the current JSON envelope.
json::Value value_to_json(const Value &v);

Value json_to_value(const json::Value &v);

json::Value value_to_json(const Value &v) {
  switch (v.kind()) {
  case Value::K_NULL:
    return json::Value();
  case Value::K_BOOL:
    return json::Value(v.as_bool());
  case Value::K_INT:
    return json::Value((double)v.as_int());
  case Value::K_DOUBLE:
    return json::Value(v.as_double());
  case Value::K_STRING:
    return json::Value(v.as_string());
  case Value::K_ARRAY: {
    json::Array a;
    for (const auto &e : v.as_array())
      a.push_back(value_to_json(e));
    return json::Value(std::move(a));
  }
  case Value::K_OBJECT: {
    json::Object o;
    for (const auto &kv : v.as_object())
      o[kv.first] = value_to_json(kv.second);
    return json::Value(std::move(o));
  }
  case Value::K_REF: {
    json::Object o;
    o["this_id"] = json::Value((double)v.as_ref().this_id);
    return json::Value(std::move(o));
  }
  }
  return json::Value();
}

Value json_to_value(const json::Value &v) {
  if (v.is_null())
    return Value();
  if (v.is_bool())
    return Value(v.as_bool());
  if (v.is_num()) {
    double d = v.as_num();
    if (d == (double)(int64_t)d)
      return Value((int64_t)d);
    return Value(d);
  }
  if (v.is_str())
    return Value(v.as_str());
  if (v.is_arr()) {
    ValueArray a;
    for (const auto &e : v.as_arr())
      a.push_back(json_to_value(e));
    return Value(a);
  }
  if (v.is_obj()) {
    const auto &o = v.as_obj();
    // Special case: {"this_id": N} -> ObjectRef
    if (o.size() == 1 && o.count("this_id") && o.at("this_id").is_num()) {
      return Value(ObjectRef((int)o.at("this_id").as_num()));
    }
    ValueObject obj;
    for (const auto &kv : o)
      obj[kv.first] = json_to_value(kv.second);
    return Value(obj);
  }
  return Value();
}

} // namespace

// ---------------------------------------------------------------
// RunScriptClientImpl
// ---------------------------------------------------------------

class RunScriptClientImpl : public IRunScriptClient {
public:
  RunScriptClientImpl(std::string host, uint16_t port) : host_(std::move(host)), port_(port) {
  }

  ~RunScriptClientImpl() override {
    disconnect();
  }

  void add_global_function(std::shared_ptr<RunScriptClientGlobalFunction> fn) override {
    if (!fn)
      throw std::runtime_error("add_global_function: null");
    global_functions_[fn->name()] = fn;
    if (connected_)
      send_register_function(fn->name());
  }

  void register_object_type(std::shared_ptr<RunScriptClientObjectType> factory) override {
    if (!factory)
      throw std::runtime_error("register_object_type: null");
    object_types_[factory->name()] = factory;
    if (connected_)
      send_register_object_type(factory);
  }

  void add_global_object(std::shared_ptr<RunScriptClientGlobalObject> obj) override {
    if (!obj)
      throw std::runtime_error("add_global_object: null");
    if (!ensure_connected())
      throw std::runtime_error("add_global_object: not connected");
    int id = register_object_impl(obj);
    send_register_object(obj, id);
    global_objects_[obj->name()] = obj;
  }

  int register_object(std::shared_ptr<RunScriptClientObject> obj) override {
    return register_object_impl(obj);
  }

  Value run(const std::string &script, const std::string &entry, const std::vector<Value> &args) override {
    if (!ensure_connected())
      throw std::runtime_error("run: not connected");

    json::Object req;
    req["script"] = json::Value(script);
    req["entry"] = json::Value(entry);
    json::Array jargs;
    for (const auto &a : args)
      jargs.push_back(value_to_json(a));
    req["args"] = json::Value(std::move(jargs));

    if (!write_frame(json::dump(json::Value(req))))
      throw std::runtime_error("run: write failed");

    while (true) {
      std::string frame;
      if (!read_frame(frame))
        throw std::runtime_error("run: connection closed");

      json::Value msg = json::parse(frame);
      if (!msg.is_obj())
        continue;

      const auto &o = msg.as_obj();
      std::string type = o.count("type") ? o.at("type").as_str() : "exec";

      if (type == "callback") {
        serve_callback(o);
        continue;
      }

      // exec result
      if (o.count("ok") && o.at("ok").is_bool() && !o.at("ok").as_bool()) {
        std::string err = o.count("error") ? o.at("error").as_str() : "unknown error";
        throw std::runtime_error("run: " + err);
      }
      return o.count("value") ? json_to_value(o.at("value")) : Value();
    }
  }

private:
  // ---- connection ----

  bool ensure_connected() {
    if (connected_)
      return true;

    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0)
      return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) != 1) {
      ::close(fd_);
      fd_ = -1;
      return false;
    }
    if (::connect(fd_, (sockaddr *)&addr, sizeof(addr)) < 0) {
      ::close(fd_);
      fd_ = -1;
      return false;
    }
    proto::set_recv_timeout(fd_, 30'000);
    connected_ = true;

    // Re-send registrations that were added before connect.
    for (auto &kv : global_functions_)
      if (!send_register_function(kv.first))
        return false;
    for (auto &kv : object_types_)
      if (!send_register_object_type(kv.second))
        return false;
    for (auto &kv : global_objects_) {
      // already registered when added; skip
    }

    return true;
  }

  void disconnect() {
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
    connected_ = false;
    objects_.clear();
    next_object_id_ = 0;
  }

  bool read_frame(std::string &out) {
    auto frame = proto::read_frame(fd_);
    if (!frame)
      return false;
    out = std::move(*frame);
    return true;
  }

  bool write_frame(const std::string &data) {
    return proto::write_frame(fd_, data);
  }

  // ---- registration ----

  bool send_register_function(const std::string &name) {
    json::Object req;
    req["type"] = json::Value(std::string("register_function"));
    req["name"] = json::Value(name);
    return send_register_and_wait(json::Value(std::move(req)));
  }

  bool send_register_object_type(std::shared_ptr<RunScriptClientObjectType> factory) {
    json::Object req;
    req["type"] = json::Value(std::string("register_object_type"));
    req["name"] = json::Value(factory->name());
    json::Array methods;
    for (const auto &m : factory->methods())
      methods.push_back(json::Value(m));
    req["methods"] = json::Value(std::move(methods));
    return send_register_and_wait(json::Value(std::move(req)));
  }

  bool send_register_object(std::shared_ptr<RunScriptClientGlobalObject> obj, int id) {
    json::Object req;
    req["type"] = json::Value(std::string("register_object"));
    req["name"] = json::Value(obj->name());
    req["this_id"] = json::Value((double)id);
    req["type_name"] = json::Value(obj->type());
    json::Array methods;
    for (const auto &m : obj->methods())
      methods.push_back(json::Value(m));
    req["methods"] = json::Value(std::move(methods));
    return send_register_and_wait(json::Value(std::move(req)));
  }

  bool send_register_and_wait(const json::Value &req) {
    if (!write_frame(json::dump(req)))
      return false;
    std::string frame;
    if (!read_frame(frame))
      return false;
    json::Value resp = json::parse(frame);
    if (!resp.is_obj())
      return false;
    const auto &o = resp.as_obj();
    if (o.count("ok") && o.at("ok").is_bool() && !o.at("ok").as_bool()) {
      std::string err = o.count("error") ? o.at("error").as_str() : "registration failed";
      throw std::runtime_error("register: " + err);
    }
    return true;
  }

  // ---- object registry ----

  int register_object_impl(std::shared_ptr<RunScriptClientObject> obj) {
    if (!obj)
      throw std::runtime_error("register_object: null");
    int id = ++next_object_id_;
    obj->set_this_id(id);
    objects_[id] = obj;
    return id;
  }

  // ---- callback dispatch ----

  void serve_callback(const json::Object &o) {
    int id = (int)o.at("id").as_num();
    std::string method = o.at("method").as_str();
    std::vector<Value> args;
    if (o.count("args") && o.at("args").is_arr())
      for (const auto &a : o.at("args").as_arr())
        args.push_back(json_to_value(a));

    json::Object resp;
    resp["type"] = json::Value(std::string("callback_result"));
    resp["id"] = json::Value((double)id);
    try {
      resp["ok"] = json::Value(true);
      resp["value"] = value_to_json(dispatch(method, args));
    } catch (const std::exception &e) {
      resp["ok"] = json::Value(false);
      resp["error"] = json::Value(std::string(e.what()));
    }
    write_frame(json::dump(json::Value(resp)));
  }

  Value dispatch(const std::string &method, const std::vector<Value> &args) {
    // 1. Global function (no this_id)
    auto fit = global_functions_.find(method);
    if (fit != global_functions_.end())
      return fit->second->execute(*this, args);

    // 2. Global object method: method is "<obj>.<m>" or
    //    the server passes the object name as the first arg.
    //    We use the convention: method == "Some.GetFile" for globals.
    auto dot = method.find('.');
    if (dot != std::string::npos) {
      std::string obj_name = method.substr(0, dot);
      std::string m = method.substr(dot + 1);
      auto oit = global_objects_.find(obj_name);
      if (oit != global_objects_.end())
        return oit->second->execute(*this, m, args);
    }

    // 3. Instance method: first arg is this_id.
    if (!args.empty() && args[0].is_int()) {
      int this_id = (int)args[0].as_int();
      auto oit = objects_.find(this_id);
      if (oit != objects_.end()) {
        std::vector<Value> rest(args.begin() + 1, args.end());
        return oit->second->execute(method, rest);
      }
    }

    throw std::runtime_error("unknown callback: " + method);
  }

  // ---- fields ----

  std::string host_;
  uint16_t port_;
  int fd_ = -1;
  bool connected_ = false;

  int next_object_id_ = 0;

  std::map<std::string, std::shared_ptr<RunScriptClientGlobalFunction>> global_functions_;
  std::map<std::string, std::shared_ptr<RunScriptClientObjectType>> object_types_;
  std::map<std::string, std::shared_ptr<RunScriptClientGlobalObject>> global_objects_;
  std::map<int, std::shared_ptr<RunScriptClientObject>> objects_;
};

std::unique_ptr<IRunScriptClient> createS5RunClient(const std::string &host, uint16_t port) {
  return std::unique_ptr<IRunScriptClient>(new RunScriptClientImpl(host, port));
}

} // namespace example