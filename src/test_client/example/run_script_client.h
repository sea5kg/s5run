// AUTO-GENERATED. DO NOT EDIT.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "value.h"

namespace example {

class IRunScriptClient;

// Base class for host functions callable from a script.
// Derive from it, pass the script-visible name to the constructor,
// and implement execute().
class RunScriptClientGlobalFunction {
private:
  std::string m_name;

public:
  explicit RunScriptClientGlobalFunction(const std::string &name) : m_name(name) {
  }
  virtual ~RunScriptClientGlobalFunction() {
  }
  std::string name() const {
    return m_name;
  }
  virtual Value execute(IRunScriptClient &client, const std::vector<Value> &args) = 0;
};

class RunScriptClientObject {
private:
  std::string m_type;
  int m_this_id;

public:
  explicit RunScriptClientObject(const std::string &type) : m_type(type) {};
  virtual ~RunScriptClientObject() {
  }

  std::string type() const {
    return m_type;
  }
  virtual int this_id() const {
    return m_this_id;
  };
  virtual void set_this_id(int id) {
    m_this_id = id;
  };
  virtual std::vector<std::string> methods() const = 0;
  virtual Value execute(IRunScriptClient &client, const std::string &method, const std::vector<Value> &args) = 0;
};

class RunScriptClientGlobalObject {
private:
  std::string m_name;

public:
  explicit RunScriptClientGlobalObject(const std::string &name) : m_name(name) {
  }
  virtual ~RunScriptClientGlobalObject() {
  }

  std::string name() const {
    return m_name;
  }

  virtual std::vector<std::string> methods() const = 0;
  virtual Value execute(IRunScriptClient &client, const std::string &method, const std::vector<Value> &args) = 0;
};

class RunScriptClientObjectType {
private:
  std::string m_name;

public:
  explicit RunScriptClientObjectType(const std::string &name) : m_name(name) {
  }
  virtual ~RunScriptClientObjectType() {
  }
  std::string name() const {
    return m_name;
  }
  virtual std::shared_ptr<RunScriptClientGlobalObject>
  create(IRunScriptClient &client, const std::vector<Value> &args) = 0;
};

// TCP client for run-scripts-server.
class IRunScriptClient {
public:
  virtual ~IRunScriptClient() = default;
  virtual void add_global_function(std::shared_ptr<RunScriptClientGlobalFunction> fn) = 0;
  virtual void add_global_object(const std::string &name, std::shared_ptr<RunScriptClientObject> obj) = 0;
  virtual void register_object_type(std::shared_ptr<RunScriptClientObjectType> factory) = 0;

  // Register an aggregate object instance and get its wire id.
  // Calls obj->set_this_id(id) before returning.
  virtual int register_object(std::shared_ptr<RunScriptClientObject> obj) = 0;

  virtual Value run(const std::string &script, const std::string &entry, const std::vector<Value> &args) = 0;
};

std::unique_ptr<IRunScriptClient> createRunScriptClient(const std::string &host, uint16_t port);

} // namespace example