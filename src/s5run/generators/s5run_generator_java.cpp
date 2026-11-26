/**********************************************************************************
 *
 *            ░░░░░░░
 *           ░██▓▓▓▓▒
 *  ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 *  ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *   ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 *  ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *   ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
 *
 * MIT License
 *
 * Copyright (c) 2026 Evgenii Sopov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Original repository: https://github.com/sea5kg/s5run
 *
 ***********************************************************************************/

#include "s5run_generators.h"
#include "s5run_schema.h"

#include <sys/stat.h>

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace s5run {
namespace generators {

// -----------------------------------------------------------------------
// Java dialect: 21 (switch patterns), 17 (instanceof binding),
// 11 (classic casts) share everything except localized snippets.
// -----------------------------------------------------------------------

enum java_flavor {
  JAVA_11,
  JAVA_17,
  JAVA_21,
};

static bool j_is_scalar(const std::string &t) {
  return t == "bool" || t == "int" || t == "uint" || t == "float" || t == "double" || t == "string" || t == "byte";
}

static bool j_is_void(const std::string &t) {
  return t == "void";
}

static bool j_is_array(const std::string &t) {
  return t.size() > 7 && t.substr(0, 6) == "array<" && t[t.size() - 1] == '>';
}

static std::string j_array_inner(const std::string &t) {
  return t.substr(6, t.size() - 7);
}

static bool j_is_dynamic(const std::string &t) {
  return t == "var";
}

static bool j_is_ref(const std::string &t) {
  return !j_is_scalar(t) && !j_is_dynamic(t) && !j_is_void(t) && !j_is_array(t);
}

static bool j_has_convert(const std::vector<schema_argument> &args) {
  return args.size() == 1 && !args[0].convert_from.empty();
}

static std::string j_scalar(const std::string &t) {
  if (t == "bool")
    return "boolean";
  if (t == "int" || t == "uint")
    return "long";
  if (t == "float" || t == "double")
    return "double";
  if (t == "string")
    return "String";
  if (t == "byte")
    return "byte";
  return t;
}

static std::string j_boxed(const std::string &t) {
  if (t == "long")
    return "Long";
  if (t == "boolean")
    return "Boolean";
  if (t == "double")
    return "Double";
  if (t == "byte")
    return "Byte";
  return t;
}

static std::string type_to_java(const std::string &t) {
  if (j_is_dynamic(t))
    return "S5RunVariable";
  if (j_is_array(t)) {
    std::string inner = type_to_java(j_array_inner(t));
    return "List<" + j_boxed(inner) + ">";
  }
  return j_scalar(t);
}

static std::string j_read_expr(const std::string &t);

static std::string j_suffix(const std::string &t) {
  if (j_is_dynamic(t)) {
    return "S5RunVariable";
  }
  if (j_is_scalar(t)) {
    if (t == "bool")
      return "Bool";
    if (t == "int" || t == "uint")
      return "Long";
    if (t == "float" || t == "double")
      return "Double";
    if (t == "string")
      return "Str";
    if (t == "byte")
      return "Byte";
  }
  return "Of" + j_suffix(j_array_inner(t));
}

static std::string j_read_expr(const std::string &t) {
  if (j_is_scalar(t)) {
    if (t == "int" || t == "uint")
      return "r.readLongInt()";
    if (t == "bool")
      return "r.readBool()";
    if (t == "float" || t == "double")
      return "r.readDouble()";
    if (t == "string")
      return "r.strValue()";
    if (t == "byte")
      return "r.readByteNum()";
  }
  if (j_is_dynamic(t))
    return "S5RunWire.decodeValue(r)";
  return "S5RunWire.readList" + j_suffix(t) + "(r)";
}

static std::string j_write_stmt(const std::string &t, const std::string &expr) {
  if (j_is_scalar(t)) {
    if (t == "int" || t == "uint")
      return "w.int64(" + expr + ");";
    if (t == "bool")
      return "w.boolValue(" + expr + ");";
    if (t == "float" || t == "double")
      return "w.f64Value(" + expr + ");";
    if (t == "string")
      return "w.strValue(" + expr + ");";
    if (t == "byte")
      return "w.byteValue((int) " + expr + ");";
  }
  if (j_is_dynamic(t))
    return "S5RunWire.encodeValue(w, " + expr + ");";
  return "S5RunWire.writeList" + j_suffix(t) + "(w, " + expr + ");";
}

// -----------------------------------------------------------------------
// Shared runtime interface (methods/functions)
// -----------------------------------------------------------------------

struct j_method_like {
  std::string name;
  std::string return_type;
  std::vector<schema_argument> arguments;
};

static std::vector<j_method_like> all_functions(const schema &s) {
  std::vector<j_method_like> out;
  for (const auto &f : s.functions)
    out.push_back({f.name, f.return_type, f.arguments});
  return out;
}

static std::vector<j_method_like> all_methods(const schema &s) {
  std::vector<j_method_like> out;
  for (const auto &c : s.classes)
    for (const auto &m : c.methods)
      out.push_back({c.name + "::" + m.name, m.return_type, m.arguments});
  return out;
}

static std::vector<std::string> list_helpers_needed(const schema &s) {
  std::vector<std::string> out;
  std::set<std::string> seen;
  std::vector<j_method_like> all = all_methods(s);
  for (const auto &fn : all_functions(s))
    all.push_back(fn);
  for (const auto &fm : all) {
    for (const auto &a : fm.arguments) {
      if (!a.convert_from.empty())
        continue;
      if (j_is_array(a.type) && !j_is_ref(j_array_inner(a.type))) {
        if (seen.count(a.type) == 0) {
          seen.insert(a.type);
          out.push_back(a.type);
        }
      }
    }
    if (!j_is_void(fm.return_type)) {
      if (j_is_array(fm.return_type) && !j_is_ref(j_array_inner(fm.return_type))) {
        if (seen.count(fm.return_type) == 0) {
          seen.insert(fm.return_type);
          out.push_back(fm.return_type);
        }
      }
    }
  }
  return out;
}

static bool type_uses_refs(const std::string &t) {
  return j_is_ref(t) || (j_is_array(t) && j_is_ref(j_array_inner(t)));
}

static bool needs_refs(const schema &s) {
  std::vector<j_method_like> all = all_methods(s);
  for (const auto &fn : all_functions(s))
    all.push_back(fn);
  for (const auto &fm : all) {
    for (const auto &a : fm.arguments) {
      if (!a.convert_from.empty())
        continue;
      if (type_uses_refs(a.type))
        return true;
    }
    if (type_uses_refs(fm.return_type))
      return true;
  }
  return false;
}

static std::string j_replace_all(std::string s, const std::string &from, const std::string &to) {
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != std::string::npos) {
    s.replace(pos, from.size(), to);
    pos += to.size();
  }
  return s;
}

// -----------------------------------------------------------------------
// S5RunContext decode/encode blocks (per-schema, shared by all flavors)
// -----------------------------------------------------------------------

static std::string decode_args(const j_method_like &fm) {
  std::vector<std::string> lines;
  if (j_has_convert(fm.arguments)) {
    const schema_argument &a = fm.arguments[0];
    lines.push_back("            long n_" + a.name + " = r.u32();");
    lines.push_back("            List<String> " + a.name + " = new ArrayList<>((int) n_" + a.name + ");");
    lines.push_back("            for (long i = 0; i < n_" + a.name + "; i++)");
    lines.push_back("                " + a.name + ".add(valueToString(S5RunWire.decodeValue(r)));");
    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) {
      if (i > 0)
        out += "\n";
      out += lines[i];
    }
    return out;
  }
  for (const auto &a : fm.arguments) {
    const std::string &t = a.type;
    if (j_is_array(t) && j_is_ref(j_array_inner(t))) {
      std::string cls = j_array_inner(t);
      lines.push_back("            if (r.u8() != S5RunWire.T_ARRAY) throw new RuntimeException(\"wire: expected array\");");
      lines.push_back("            int cnt_" + a.name + " = (int) r.u32();");
      lines.push_back("            List<" + cls + "> " + a.name + " = new ArrayList<>(cnt_" + a.name + ");");
      lines.push_back("            for (int i = 0; i < cnt_" + a.name + "; i++) {");
      lines.push_back("                int refId = readRefId(r);");
      lines.push_back("                " + cls + " obj = instanceById(\"" + cls + "\", refId);");
      lines.push_back("                " + a.name + ".add(obj);");
      lines.push_back("            }");
    } else if (j_is_ref(t)) {
      lines.push_back("            int refId_" + a.name + " = readRefId(r);");
      lines.push_back("            " + t + " " + a.name + " = instanceById(\"" + t + "\", refId_" + a.name + ");");
    } else {
      lines.push_back("            " + type_to_java(t) + " " + a.name + " = " + j_read_expr(t) + ";");
    }
  }
  std::string out;
  for (size_t i = 0; i < lines.size(); ++i) {
    if (i > 0)
      out += "\n";
    out += lines[i];
  }
  return out;
}

static std::string call_args(const j_method_like &fm) {
  std::string out;
  for (const auto &a : fm.arguments) {
    if (!out.empty())
      out += ", ";
    out += a.name;
  }
  return out;
}

static std::string encode_result(const j_method_like &fm) {
  if (j_is_void(fm.return_type))
    return "            w.u8(S5RunWire.T_UNDEFINED);";
  const std::string &t = fm.return_type;
  if (j_is_array(t) && j_is_ref(j_array_inner(t))) {
    std::string cls = j_array_inner(t);
    std::string out = "            w.u8(S5RunWire.T_ARRAY);\n";
    out += "            w.u32(res.size());\n";
    out += "            for (" + cls + " item : res) {\n";
    out += "                int id = registerInstance(item);\n";
    out += "                w.u8(S5RunWire.T_REF);\n";
    out += "                w.i32(id);\n";
    out += "                w.str(\"" + cls + "\");\n";
    out += "            }";
    return out;
  }
  if (j_is_ref(t)) {
    std::string out = "            int objId = registerInstance(res);\n";
    out += "            w.u8(S5RunWire.T_REF);\n";
    out += "            w.i32(objId);\n";
    out += "            w.str(\"" + t + "\");";
    return out;
  }
  return "            " + j_write_stmt(t, "res");
}

// -----------------------------------------------------------------------
// Dialect snippets
// -----------------------------------------------------------------------

static std::string encode_value_snippet(java_flavor f);
static std::string value_to_string_snippet(java_flavor f);
static std::string decode_value_snippet(java_flavor f);
static std::string read_long_int_snippet(java_flavor f);
static std::string str_switch_snippet(java_flavor f);
static std::string value_file_snippet(java_flavor f);
static std::string run_result_file_snippet(java_flavor f);
static std::string dispatch_case(java_flavor f, const std::string &name);
static std::string dispatch_default(java_flavor f);
static std::string pending_decls(java_flavor f);
static std::string flush_pending_body(java_flavor f);
static std::string frame_decl(java_flavor f);

static std::string encode_value_snippet(java_flavor f) {
  if (f == JAVA_21) {
    return R"JAVA(
    public static void encodeValue(Writer w, S5RunVariable v) {
        switch (v) {
            case S5RunVariable.Null n -> w.u8(T_UNDEFINED);
            case S5RunVariable.Undefined u -> w.u8(T_UNDEFINED);
            case S5RunVariable.Bool b -> { w.u8(T_BOOL); w.bool(b.v()); }
            case S5RunVariable.Int i -> w.int64(i.v());
            case S5RunVariable.Dbl d -> { w.u8(T_FLOAT64); w.f64(d.v()); }
            case S5RunVariable.Str s -> w.strValue(s.v());
            case S5RunVariable.Arr a -> {
                w.u8(T_ARRAY);
                w.u32(a.v().size());
                for (S5RunVariable e : a.v()) encodeValue(w, e);
            }
            case S5RunVariable.Obj o -> {
                w.u8(T_OBJECT);
                w.u32(o.v().size());
                for (var en : o.v().entrySet()) {
                    w.strValue(en.getKey());
                    encodeValue(w, en.getValue());
                }
            }
            case S5RunVariable.Ref rf -> {
                w.u8(T_REF);
                w.i32(rf.thisId());
                w.str(rf.type());
            }
        }
    })JAVA";
  }
  if (f == JAVA_17) {
    return R"JAVA(
    public static void encodeValue(Writer w, S5RunVariable v) {
        if (v instanceof S5RunVariable.Null) {
            w.u8(T_UNDEFINED);
        } else if (v instanceof S5RunVariable.Undefined) {
            w.u8(T_UNDEFINED);
        } else if (v instanceof S5RunVariable.Bool b) {
            w.u8(T_BOOL); w.bool(b.v());
        } else if (v instanceof S5RunVariable.Int i) {
            w.int64(i.v());
        } else if (v instanceof S5RunVariable.Dbl d) {
            w.u8(T_FLOAT64); w.f64(d.v());
        } else if (v instanceof S5RunVariable.Str s) {
            w.strValue(s.v());
        } else if (v instanceof S5RunVariable.Arr a) {
            w.u8(T_ARRAY);
            w.u32(a.v().size());
            for (S5RunVariable e : a.v()) encodeValue(w, e);
        } else if (v instanceof S5RunVariable.Obj o) {
            w.u8(T_OBJECT);
            w.u32(o.v().size());
            for (var en : o.v().entrySet()) {
                w.strValue(en.getKey());
                encodeValue(w, en.getValue());
            }
        } else if (v instanceof S5RunVariable.Ref rf) {
            w.u8(T_REF);
            w.i32(rf.thisId());
            w.str(rf.type());
        }
    })JAVA";
  }
  return R"JAVA(
    public static void encodeValue(Writer w, S5RunVariable v) {
        if (v instanceof S5RunVariable.Null) {
            w.u8(T_UNDEFINED);
        } else if (v instanceof S5RunVariable.Undefined) {
            w.u8(T_UNDEFINED);
        } else if (v instanceof S5RunVariable.Bool) {
            w.u8(T_BOOL); w.bool(((S5RunVariable.Bool) v).v());
        } else if (v instanceof S5RunVariable.Int) {
            w.int64(((S5RunVariable.Int) v).v());
        } else if (v instanceof S5RunVariable.Dbl) {
            w.u8(T_FLOAT64); w.f64(((S5RunVariable.Dbl) v).v());
        } else if (v instanceof S5RunVariable.Str) {
            w.strValue(((S5RunVariable.Str) v).v());
        } else if (v instanceof S5RunVariable.Arr) {
            S5RunVariable.Arr a = (S5RunVariable.Arr) v;
            w.u8(T_ARRAY);
            w.u32(a.v().size());
            for (S5RunVariable e : a.v()) encodeValue(w, e);
        } else if (v instanceof S5RunVariable.Obj) {
            S5RunVariable.Obj o = (S5RunVariable.Obj) v;
            w.u8(T_OBJECT);
            w.u32(o.v().size());
            for (Map.Entry<String, S5RunVariable> en : o.v().entrySet()) {
                w.strValue(en.getKey());
                encodeValue(w, en.getValue());
            }
        } else if (v instanceof S5RunVariable.Ref) {
            S5RunVariable.Ref rf = (S5RunVariable.Ref) v;
            w.u8(T_REF);
            w.i32(rf.thisId());
            w.str(rf.type());
        }
    })JAVA";
}

static std::string value_to_string_snippet(java_flavor f) {
  if (f == JAVA_21) {
    return R"JAVA(
    public static String valueToString(S5RunVariable v) {
        return switch (v) {
            case S5RunVariable.Undefined u -> "undefined";
            case S5RunVariable.Null n -> "null";
            case S5RunVariable.Bool b -> Boolean.toString(b.v());
            case S5RunVariable.Int i -> Long.toString(i.v());
            case S5RunVariable.Dbl d -> Double.toString(d.v());
            case S5RunVariable.Str s -> s.v();
            default -> throw new RuntimeException("wire: expected a scalar argument");
        };
    })JAVA";
  }
  if (f == JAVA_17) {
    return R"JAVA(
    public static String valueToString(S5RunVariable v) {
        if (v instanceof S5RunVariable.Undefined) return "undefined";
        if (v instanceof S5RunVariable.Null) return "null";
        if (v instanceof S5RunVariable.Bool b) return Boolean.toString(b.v());
        if (v instanceof S5RunVariable.Int i) return Long.toString(i.v());
        if (v instanceof S5RunVariable.Dbl d) return Double.toString(d.v());
        if (v instanceof S5RunVariable.Str s) return s.v();
        throw new RuntimeException("wire: expected a scalar argument");
    })JAVA";
  }
  return R"JAVA(
    public static String valueToString(S5RunVariable v) {
        if (v instanceof S5RunVariable.Undefined) return "undefined";
        if (v instanceof S5RunVariable.Null) return "null";
        if (v instanceof S5RunVariable.Bool) return Boolean.toString(((S5RunVariable.Bool) v).v());
        if (v instanceof S5RunVariable.Int) return Long.toString(((S5RunVariable.Int) v).v());
        if (v instanceof S5RunVariable.Dbl) return Double.toString(((S5RunVariable.Dbl) v).v());
        if (v instanceof S5RunVariable.Str) return ((S5RunVariable.Str) v).v();
        throw new RuntimeException("wire: expected a scalar argument");
    })JAVA";
}

static std::string decode_value_snippet(java_flavor f) {
  if (f == JAVA_21 || f == JAVA_17) {
    return R"JAVA(    public static S5RunVariable decodeValue(Reader r) {
        int tag = r.u8();
        return switch (tag) {
            case T_UNDEFINED -> new S5RunVariable.Undefined();
            case T_BOOL -> new S5RunVariable.Bool(r.readBoolRaw());
            case T_INT8 -> new S5RunVariable.Int(r.readI8());
            case T_INT16 -> new S5RunVariable.Int(r.readI16());
            case T_INT32, T_UINT32 -> new S5RunVariable.Int(r.i32());
            case T_INT64, T_UINT64 -> new S5RunVariable.Int(r.i64());
            case T_UINT8 -> new S5RunVariable.Int(r.u8());
            case T_UINT16 -> new S5RunVariable.Int(r.u16());
            case T_FLOAT32 -> new S5RunVariable.Dbl(r.f32());
            case T_FLOAT64 -> new S5RunVariable.Dbl(r.f64());
            case T_STRING -> new S5RunVariable.Str(r.str());
            case T_ARRAY -> {
                int n = (int) r.u32();
                List<S5RunVariable> a = new ArrayList<>(n);
                for (int i = 0; i < n; i++) a.add(decodeValue(r));
                yield new S5RunVariable.Arr(a);
            }
            case T_OBJECT -> {
                int n = (int) r.u32();
                Map<String, S5RunVariable> o = new LinkedHashMap<>();
                for (int i = 0; i < n; i++) o.put(r.strValue(), decodeValue(r));
                yield new S5RunVariable.Obj(o);
            }
            case T_REF -> new S5RunVariable.Ref(r.i32(), r.str());
            default -> throw new RuntimeException("wire: unknown value tag " + tag);
        };
    })JAVA";
  }
  return R"JAVA(    public static S5RunVariable decodeValue(Reader r) {
        int tag = r.u8();
        switch (tag) {
            case T_UNDEFINED: return S5RunVariable.Undefined.INSTANCE;
            case T_BOOL: return new S5RunVariable.Bool(r.readBoolRaw());
            case T_INT8: return new S5RunVariable.Int(r.readI8());
            case T_INT16: return new S5RunVariable.Int(r.readI16());
            case T_INT32: case T_UINT32: return new S5RunVariable.Int(r.i32());
            case T_INT64: case T_UINT64: return new S5RunVariable.Int(r.i64());
            case T_UINT8: return new S5RunVariable.Int(r.u8());
            case T_UINT16: return new S5RunVariable.Int(r.u16());
            case T_FLOAT32: return new S5RunVariable.Dbl(r.f32());
            case T_FLOAT64: return new S5RunVariable.Dbl(r.f64());
            case T_STRING: return new S5RunVariable.Str(r.str());
            case T_ARRAY: {
                int n = (int) r.u32();
                List<S5RunVariable> a = new ArrayList<>(n);
                for (int i = 0; i < n; i++) a.add(decodeValue(r));
                return new S5RunVariable.Arr(a);
            }
            case T_OBJECT: {
                int n = (int) r.u32();
                Map<String, S5RunVariable> o = new LinkedHashMap<>();
                for (int i = 0; i < n; i++) o.put(r.strValue(), decodeValue(r));
                return new S5RunVariable.Obj(o);
            }
            case T_REF: return new S5RunVariable.Ref(r.i32(), r.str());
            default: throw new RuntimeException("wire: unknown value tag " + tag);
        }
    })JAVA";
}

static std::string read_long_int_snippet(java_flavor f) {
  if (f == JAVA_21 || f == JAVA_17) {
    return R"JAVA(        public long readLongInt() {
            int tag = u8();
            return switch (tag) {
                case T_INT8 -> (byte) u8();
                case T_INT16 -> (short) u16();
                case T_INT32, T_UINT32 -> (long) i32();
                case T_INT64, T_UINT64 -> i64();
                case T_UINT8 -> u8();
                case T_UINT16 -> u16();
                default -> throw new RuntimeException("wire: expected an integer");
            };
        })JAVA";
  }
  return R"JAVA(        public long readLongInt() {
            int tag = u8();
            switch (tag) {
                case T_INT8: return (byte) u8();
                case T_INT16: return (short) u16();
                case T_INT32: case T_UINT32: return (long) i32();
                case T_INT64: case T_UINT64: return i64();
                case T_UINT8: return u8();
                case T_UINT16: return u16();
                default: throw new RuntimeException("wire: expected an integer");
            }
        })JAVA";
}

static std::string str_switch_snippet(java_flavor f) {
  if (f == JAVA_21 || f == JAVA_17) {
    return R"JAVA(        public String str() {
            int lenSize = u8();
            int n = switch (lenSize) {
                case 0x00 -> u8();
                case 0x01 -> u16();
                case 0x02 -> (int) u32();
                default -> throw new RuntimeException("wire: bad string length size");
            };
            byte[] b = new byte[n];
            buf.get(b);
            return new String(b, StandardCharsets.UTF_8);
        })JAVA";
  }
  return R"JAVA(        public String str() {
            int lenSize = u8();
            int n;
            switch (lenSize) {
                case 0x00: n = u8(); break;
                case 0x01: n = u16(); break;
                case 0x02: n = (int) u32(); break;
                default: throw new RuntimeException("wire: bad string length size");
            }
            byte[] b = new byte[n];
            buf.get(b);
            return new String(b, StandardCharsets.UTF_8);
        })JAVA";
}

static std::string value_file_snippet(java_flavor f) {
  if (f == JAVA_21 || f == JAVA_17) {
    return R"JAVA(
public sealed interface S5RunVariable {
    record Null() implements S5RunVariable {}
    record Undefined() implements S5RunVariable {}
    record Bool(boolean v) implements S5RunVariable {}
    record Int(long v) implements S5RunVariable {}
    record Dbl(double v) implements S5RunVariable {}
    record Str(String v) implements S5RunVariable {}
    record Arr(List<S5RunVariable> v) implements S5RunVariable {}
    record Obj(Map<String, S5RunVariable> v) implements S5RunVariable {}
    record Ref(int thisId, String type) implements S5RunVariable {}

    /** Structured multi-line representation, handy for tests and debugging, e.g.
     * K_INT 1024 / K_OBJECT BEGIN..END blocks. Object fields are sorted by name. */
    default String toStringS5Run() {
        StringBuilder sb = new StringBuilder();
        appendS5Run(sb, 0);
        String out = sb.toString();
        if (out.endsWith("\n")) {
            out = out.substring(0, out.length() - 1);
        }
        return out;
    }

    /** JSON representation (hand-rolled, no external libraries). */
    default String toStringJson() {
        StringBuilder sb = new StringBuilder();
        appendJson(sb);
        return sb.toString();
    }

    private void appendS5Run(StringBuilder sb, int indent) {
        String pad = " ".repeat(indent);
        String padIn = " ".repeat(indent + 2);
        if (this instanceof Undefined) {
            sb.append("K_UNDEFINED\n");
        } else if (this instanceof Null) {
            sb.append("K_NULL\n");
        } else if (this instanceof Bool) {
            sb.append(((Bool) this).v() ? "K_BOOL true\n" : "K_BOOL false\n");
        } else if (this instanceof Int) {
            sb.append("K_INT ").append(((Int) this).v()).append("\n");
        } else if (this instanceof Dbl) {
            sb.append("K_DOUBLE ").append(((Dbl) this).v()).append("\n");
        } else if (this instanceof Str) {
            sb.append("K_STRING ");
            appendJsonString(sb, ((Str) this).v());
            sb.append("\n");
        } else if (this instanceof Arr) {
            sb.append("K_ARRAY\n");
            sb.append(pad).append("BEGIN\n");
            for (S5RunVariable e : ((Arr) this).v()) {
                sb.append(padIn);
                e.appendS5Run(sb, indent + 2);
            }
            sb.append(pad).append("END\n");
        } else if (this instanceof Obj) {
            // TreeMap/LinkedHashMap iteration; std maps in tests are sorted
            sb.append("K_OBJECT\n");
            sb.append(pad).append("BEGIN\n");
            for (Map.Entry<String, S5RunVariable> en : ((Obj) this).v().entrySet()) {
                sb.append(padIn).append(en.getKey()).append(" ");
                en.getValue().appendS5Run(sb, indent + 2);
            }
            sb.append(pad).append("END\n");
        } else if (this instanceof Ref) {
            Ref rf = (Ref) this;
            sb.append("K_REF ").append(rf.thisId()).append(" ");
            appendJsonString(sb, rf.type());
            sb.append("\n");
        }
    }

    private static void appendJsonString(StringBuilder sb, String s) {
        sb.append('"');
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"': sb.append("\\\""); break;
                case '\\': sb.append("\\\\"); break;
                case '\n': sb.append("\\n"); break;
                case '\r': sb.append("\\r"); break;
                case '\t': sb.append("\\t"); break;
                default:
                    if (c < 0x20) {
                        sb.append(String.format("\\u%04x", (int) c));
                    } else {
                        sb.append(c);
                    }
            }
        }
        sb.append('"');
    }

    private void appendJson(StringBuilder sb) {
        if (this instanceof Null) {
            sb.append("null");
        } else if (this instanceof Undefined) {
            sb.append("null");
        } else if (this instanceof Bool) {
            sb.append(((Bool) this).v());
        } else if (this instanceof Int) {
            sb.append(((Int) this).v());
        } else if (this instanceof Dbl) {
            sb.append(((Dbl) this).v());
        } else if (this instanceof Str) {
            appendJsonString(sb, ((Str) this).v());
        } else if (this instanceof Arr) {
            sb.append("[");
            boolean first = true;
            for (S5RunVariable e : ((Arr) this).v()) {
                if (!first) {
                    sb.append(", ");
                }
                first = false;
                e.appendJson(sb);
            }
            sb.append("]");
        } else if (this instanceof Obj) {
            sb.append("{");
            boolean first = true;
            for (Map.Entry<String, S5RunVariable> en : ((Obj) this).v().entrySet()) {
                if (!first) {
                    sb.append(", ");
                }
                first = false;
                appendJsonString(sb, en.getKey());
                sb.append(": ");
                en.getValue().appendJson(sb);
            }
            sb.append("}");
        } else if (this instanceof Ref) {
            Ref rf = (Ref) this;
            sb.append("{\"__s5run_ref__\":{\"this_id\":").append(rf.thisId()).append(",\"type\":");
            appendJsonString(sb, rf.type());
            sb.append("}}");
        }
    }
}
)JAVA";
  }
  return R"JAVA(
public interface S5RunVariable {
    final class Null implements S5RunVariable {
        public static final Null INSTANCE = new Null();
        private Null() {}
    }
    final class Undefined implements S5RunVariable {
        public static final Undefined INSTANCE = new Undefined();
        private Undefined() {}
    }
    final class Bool implements S5RunVariable {
        final boolean val;
        public Bool(boolean v) { this.val = v; }
        public boolean v() { return val; }
    }
    final class Int implements S5RunVariable {
        final long val;
        public Int(long v) { this.val = v; }
        public long v() { return val; }
    }
    final class Dbl implements S5RunVariable {
        final double val;
        public Dbl(double v) { this.val = v; }
        public double v() { return val; }
    }
    final class Str implements S5RunVariable {
        final String val;
        public Str(String v) { this.val = v; }
        public String v() { return val; }
    }
    final class Arr implements S5RunVariable {
        final List<S5RunVariable> val;
        public Arr(List<S5RunVariable> v) { this.val = v; }
        public List<S5RunVariable> v() { return val; }
    }
    final class Obj implements S5RunVariable {
        final Map<String, S5RunVariable> val;
        public Obj(Map<String, S5RunVariable> v) { this.val = v; }
        public Map<String, S5RunVariable> v() { return val; }
    }
    final class Ref implements S5RunVariable {
        final int thisId;
        final String type;
        public Ref(int thisId, String type) { this.thisId = thisId; this.type = type; }
        public int thisId() { return thisId; }
        public String type() { return type; }
    }


    /** Structured multi-line representation, handy for tests and debugging, e.g.
     * K_INT 1024 / K_OBJECT BEGIN..END blocks. Object fields are sorted by name. */
    default String toStringS5Run() {
        StringBuilder sb = new StringBuilder();
        appendS5Run(sb, 0);
        String out = sb.toString();
        if (out.endsWith("\n")) {
            out = out.substring(0, out.length() - 1);
        }
        return out;
    }

    /** JSON representation (hand-rolled, no external libraries). */
    default String toStringJson() {
        StringBuilder sb = new StringBuilder();
        appendJson(sb);
        return sb.toString();
    }

    private void appendS5Run(StringBuilder sb, int indent) {
        String pad = " ".repeat(indent);
        String padIn = " ".repeat(indent + 2);
        if (this instanceof Undefined) {
            sb.append("K_UNDEFINED\n");
        } else if (this instanceof Null) {
            sb.append("K_NULL\n");
        } else if (this instanceof Bool) {
            sb.append(((Bool) this).v() ? "K_BOOL true\n" : "K_BOOL false\n");
        } else if (this instanceof Int) {
            sb.append("K_INT ").append(((Int) this).v()).append("\n");
        } else if (this instanceof Dbl) {
            sb.append("K_DOUBLE ").append(((Dbl) this).v()).append("\n");
        } else if (this instanceof Str) {
            sb.append("K_STRING ");
            appendJsonString(sb, ((Str) this).v());
            sb.append("\n");
        } else if (this instanceof Arr) {
            sb.append("K_ARRAY\n");
            sb.append(pad).append("BEGIN\n");
            for (S5RunVariable e : ((Arr) this).v()) {
                sb.append(padIn);
                e.appendS5Run(sb, indent + 2);
            }
            sb.append(pad).append("END\n");
        } else if (this instanceof Obj) {
            // TreeMap/LinkedHashMap iteration; std maps in tests are sorted
            sb.append("K_OBJECT\n");
            sb.append(pad).append("BEGIN\n");
            for (Map.Entry<String, S5RunVariable> en : ((Obj) this).v().entrySet()) {
                sb.append(padIn).append(en.getKey()).append(" ");
                en.getValue().appendS5Run(sb, indent + 2);
            }
            sb.append(pad).append("END\n");
        } else if (this instanceof Ref) {
            Ref rf = (Ref) this;
            sb.append("K_REF ").append(rf.thisId()).append(" ");
            appendJsonString(sb, rf.type());
            sb.append("\n");
        }
    }

    private static void appendJsonString(StringBuilder sb, String s) {
        sb.append('"');
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"': sb.append("\\\""); break;
                case '\\': sb.append("\\\\"); break;
                case '\n': sb.append("\\n"); break;
                case '\r': sb.append("\\r"); break;
                case '\t': sb.append("\\t"); break;
                default:
                    if (c < 0x20) {
                        sb.append(String.format("\\u%04x", (int) c));
                    } else {
                        sb.append(c);
                    }
            }
        }
        sb.append('"');
    }

    private void appendJson(StringBuilder sb) {
        if (this instanceof Null) {
            sb.append("null");
        } else if (this instanceof Undefined) {
            sb.append("null");
        } else if (this instanceof Bool) {
            sb.append(((Bool) this).v());
        } else if (this instanceof Int) {
            sb.append(((Int) this).v());
        } else if (this instanceof Dbl) {
            sb.append(((Dbl) this).v());
        } else if (this instanceof Str) {
            appendJsonString(sb, ((Str) this).v());
        } else if (this instanceof Arr) {
            sb.append("[");
            boolean first = true;
            for (S5RunVariable e : ((Arr) this).v()) {
                if (!first) {
                    sb.append(", ");
                }
                first = false;
                e.appendJson(sb);
            }
            sb.append("]");
        } else if (this instanceof Obj) {
            sb.append("{");
            boolean first = true;
            for (Map.Entry<String, S5RunVariable> en : ((Obj) this).v().entrySet()) {
                if (!first) {
                    sb.append(", ");
                }
                first = false;
                appendJsonString(sb, en.getKey());
                sb.append(": ");
                en.getValue().appendJson(sb);
            }
            sb.append("}");
        } else if (this instanceof Ref) {
            Ref rf = (Ref) this;
            sb.append("{\"__s5run_ref__\":{\"this_id\":").append(rf.thisId()).append(",\"type\":");
            appendJsonString(sb, rf.type());
            sb.append("}}");
        }
    }
}
)JAVA";
}

static std::string run_result_file_snippet(java_flavor f) {
  std::string head, fail_line, tail;
  if (f == JAVA_21 || f == JAVA_17) {
    head = "public record S5RunResult(\n"
           "        boolean hasError,\n"
           "        String error,\n"
           "        int errorLine,\n"
           "        long durationMs,\n"
           "        S5RunVariable value) {\n";
    fail_line = "        return new S5RunResult(true, msg, parseErrorLine(msg), durationMs, new S5RunVariable.Null());";
    tail = "}\n";
  } else {
    head = "public final class S5RunResult {\n"
           "    private final boolean hasError;\n"
           "    private final String error;\n"
           "    private final int errorLine;\n"
           "    private final long durationMs;\n"
           "    private final S5RunVariable value;\n"
           "\n"
           "    public S5RunResult(boolean hasError, String error, int errorLine, long durationMs, S5RunVariable value) {\n"
           "        this.hasError = hasError;\n"
           "        this.error = error;\n"
           "        this.errorLine = errorLine;\n"
           "        this.durationMs = durationMs;\n"
           "        this.value = value;\n"
           "    }\n"
           "\n"
           "    public boolean hasError() { return hasError; }\n"
           "    public String error() { return error; }\n"
           "    public int errorLine() { return errorLine; }\n"
           "    public long durationMs() { return durationMs; }\n"
           "    public S5RunVariable value() { return value; }\n";
    fail_line = "        return new S5RunResult(true, msg, parseErrorLine(msg), durationMs, S5RunVariable.Null.INSTANCE);";
    tail = "}\n";
  }
  return "\n" + head +
         "\n"
         "    public static S5RunResult ok(S5RunVariable value, long durationMs) {\n"
         "        return new S5RunResult(false, \"\", 0, durationMs, value);\n"
         "    }\n"
         "\n"
         "    public static S5RunResult fail(String error, long durationMs) {\n"
         "        String msg = error == null ? \"unknown error\" : error;\n" +
         fail_line +
         "\n"
         "    }\n"
         "\n"
         "    static int parseErrorLine(String error) {\n"
         "        if (error == null) return 0;\n"
         "        int pos = 0;\n"
         "        while (pos < error.length()) {\n"
         "            int colon = error.indexOf(':', pos);\n"
         "            if (colon < 0) return 0;\n"
         "            int p1 = colon + 1;\n"
         "            if (p1 >= error.length() || !Character.isDigit(error.charAt(p1))) {\n"
         "                pos = p1;\n"
         "                continue;\n"
         "            }\n"
         "            int p2 = p1;\n"
         "            while (p2 < error.length() && Character.isDigit(error.charAt(p2))) p2++;\n"
         "            if (p2 + 1 < error.length() && error.charAt(p2) == ':' && Character.isDigit(error.charAt(p2 + "
         "1))) {\n"
         "                return Integer.parseInt(error.substring(p1, p2));\n"
         "            }\n"
         "            pos = p2;\n"
         "        }\n"
         "        return 0;\n"
         "    }\n" +
         tail;
}

static std::string dispatch_case(java_flavor f, const std::string &name) {
  if (f == JAVA_11)
    return "            case \"" + name + "\": cb_" + name + "(cbId, r); return;";
  return "            case \"" + name + "\" -> cb_" + name + "(cbId, r);";
}

static std::string dispatch_default(java_flavor f) {
  if (f == JAVA_11)
    return "            default: throw new RuntimeException(\"unknown function: \" + name);";
  return "            default -> throw new RuntimeException(\"unknown function: \" + name);";
}

static std::string pending_decls(java_flavor f) {
  if (f == JAVA_21 || f == JAVA_17) {
    return "    private record PendingType(String name, List<String> methods, List<S5RunProperty> props) {}\n"
           "    private record PendingObject(String name, int thisId, List<String> methods, "
           "List<S5RunProperty> props) {}";
  }
  return "    private static final class PendingType {\n"
         "        final String name;\n"
         "        final List<String> methods;\n"
         "        final List<S5RunProperty> props;\n"
         "        PendingType(String name, List<String> methods, List<S5RunProperty> props) { this.name = name; "
         "this.methods = methods; this.props = props; }\n"
         "    }\n"
         "\n"
         "    private static final class PendingObject {\n"
         "        final String name;\n"
         "        final int thisId;\n"
         "        final List<String> methods;\n"
         "        final List<S5RunProperty> props;\n"
         "        PendingObject(String name, int thisId, List<String> methods, List<S5RunProperty> props) { "
         "this.name = name; this.thisId = thisId; this.methods = methods; this.props = props; }\n"
         "    }";
}

static std::string flush_pending_body(java_flavor f) {
  if (f == JAVA_21 || f == JAVA_17) {
    return "        for (PendingType t : new ArrayList<>(pendingTypes)) {\n"
           "            sendRegisterType(t.name(), t.methods(), t.props());\n"
           "        }\n"
           "        pendingTypes.clear();\n"
           "        for (PendingObject o : new ArrayList<>(pendingObjects)) {\n"
           "            sendRegisterObject(o.name(), o.thisId(), o.methods(), o.props());\n"
           "        }";
  }
  return "        for (PendingType t : new ArrayList<>(pendingTypes)) {\n"
         "            sendRegisterType(t.name, t.methods, t.props);\n"
         "        }\n"
         "        pendingTypes.clear();\n"
         "        for (PendingObject o : new ArrayList<>(pendingObjects)) {\n"
         "            sendRegisterObject(o.name, o.thisId, o.methods, o.props);\n"
         "        }";
}

static std::string frame_decl(java_flavor f) {
  if (f == JAVA_21 || f == JAVA_17)
    return "    private record Frame(int msgType, byte[] body) {}";
  return "    private static final class Frame {\n"
         "        final int msgType;\n"
         "        final byte[] body;\n"
         "        Frame(int msgType, byte[] body) { this.msgType = msgType; this.body = body; }\n"
         "    }";
}

// -----------------------------------------------------------------------
// File generators
// -----------------------------------------------------------------------

static std::string generate_wire(java_flavor f, const schema &s) {
  std::string list_helpers;
  for (const auto &type_str : list_helpers_needed(s)) {
    std::string inner = j_array_inner(type_str);
    std::string sfx = j_suffix(type_str);
    std::string inner_t = j_boxed(type_to_java(inner));
    list_helpers += "\n    public static List<" + inner_t + "> readList" + sfx +
                    "(Reader r) {\n"
                    "        if (r.u8() != T_ARRAY) throw new RuntimeException(\"wire: expected array\");\n"
                    "        int n = (int) r.u32();\n"
                    "        List<" +
                    inner_t +
                    "> out = new ArrayList<>(n);\n"
                    "        for (int i = 0; i < n; i++) {\n"
                    "            out.add(" +
                    j_read_expr(inner) +
                    ");\n"
                    "        }\n"
                    "        return out;\n"
                    "    }\n"
                    "\n"
                    "    public static void writeList" +
                    sfx + "(Writer w, List<" + inner_t +
                    "> v) {\n"
                    "        w.u8(T_ARRAY);\n"
                    "        w.u32(v.size());\n"
                    "        for (" +
                    type_to_java(inner) +
                    " e : v) {\n"
                    "            " +
                    j_write_stmt(inner, "e") +
                    "\n"
                    "        }\n"
                    "    }";
  }

  return R"JAVA(import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.*;

public final class S5RunWire {
    public static final int
        T_UNDEFINED = 0x00, T_BOOL = 0x01, T_INT8 = 0x02, T_INT16 = 0x03,
        T_INT32 = 0x04, T_INT64 = 0x05, T_UINT8 = 0x06, T_UINT16 = 0x07,
        T_UINT32 = 0x08, T_UINT64 = 0x09, T_FLOAT32 = 0x0A, T_FLOAT64 = 0x0B,
        T_STRING = 0x0C, T_ARRAY = 0x0D, T_OBJECT = 0x0E, T_REF = 0x0F;

    public static final int
        M_EXEC_REQ = 0x01, M_EXEC_RESP = 0x02, M_CALLBACK_REQ = 0x03,
        M_CALLBACK_RESP = 0x04, M_REGISTER_FUNC = 0x05, M_REGISTER_OBJ = 0x06,
        M_REGISTER_TYPE = 0x07, M_REGISTER_RESP = 0x08,
        M_STATS_REQ = 0x09, M_STATS_RESP = 0x0A, M_SERVER_HELLO = 0x0F;

    public static final class Writer {
        private final ByteArrayOutputStream buf = new ByteArrayOutputStream();

        public byte[] toByteArray() { return buf.toByteArray(); }

        public void u8(int v) { buf.write(v & 0xFF); }

        public void u32(long v) {
            for (int i = 0; i < 4; i++) buf.write((int) ((v >>> (8 * i)) & 0xFF));
        }

        public void u64(long v) {
            for (int i = 0; i < 8; i++) buf.write((int) ((v >>> (8 * i)) & 0xFF));
        }

        public void i32(int v) { u32(v & 0xFFFFFFFFL); }
        public void int64(long v) {
            if (v >= Byte.MIN_VALUE && v <= Byte.MAX_VALUE) { u8(T_INT8); u8((int) v); }
            else if (v >= Short.MIN_VALUE && v <= Short.MAX_VALUE) { u8(T_INT16); u16((int) v); }
            else if (v >= Integer.MIN_VALUE && v <= Integer.MAX_VALUE) { u8(T_INT32); u32(v); }
            else { u8(T_INT64); u64(v); }
        }

        public void u16(int v) {
            buf.write(v & 0xFF);
            buf.write((v >> 8) & 0xFF);
        }

        public void f64(double v) { u64(Double.doubleToLongBits(v)); }
        public void f32(float v) { u32(Float.floatToIntBits(v) & 0xFFFFFFFFL); }

        public void bool(boolean v) { u8(v ? 1 : 0); }

        public void strValue(String s) {
            u8(T_STRING);
            str(s);
        }

        public void boolValue(boolean v) {
            u8(T_BOOL);
            bool(v);
        }

        public void byteValue(int v) {
            u8(T_UINT8);
            u8(v);
        }

        public void f64Value(double v) {
            u8(T_FLOAT64);
            f64(v);
        }

        public void str(String s) {
            byte[] b = s.getBytes(StandardCharsets.UTF_8);
            if (b.length <= 0xFF) { u8(0x00); u8(b.length); }
            else if (b.length <= 0xFFFF) { u8(0x01); u16(b.length); }
            else { u8(0x02); u32(b.length); }
            buf.write(b, 0, b.length);
        }
    }

    public static final class Reader {
        private final ByteBuffer buf;

        public Reader(byte[] data) { buf = ByteBuffer.wrap(data); }

        public int u8() { return buf.get() & 0xFF; }
        public int u16() { return (buf.get() & 0xFF) | ((buf.get() & 0xFF) << 8); }
        public long u32() {
            long v = 0;
            for (int i = 0; i < 4; i++) v |= (buf.get() & 0xFFL) << (8 * i);
            return v;
        }
        public long u64() {
            long v = 0;
            for (int i = 0; i < 8; i++) v |= (buf.get() & 0xFFL) << (8 * i);
            return v;
        }
        public int i32() { return (int) u32(); }
        public long i64() { return u64(); }
        public int readI8() { return (byte) u8(); }
        public int readI16() { return (short) u16(); }
)JAVA" + read_long_int_snippet(f) +
         R"JAVA(
        public byte readByteNum() {
            if (u8() != T_UINT8) throw new RuntimeException("wire: expected a byte");
            return (byte) u8();
        }
        public boolean readBool() {
            if (u8() != T_BOOL) throw new RuntimeException("wire: expected a bool");
            return u8() != 0;
        }
        public boolean readBoolRaw() { return u8() != 0; }
        public double f32() { return Float.intBitsToFloat(i32()); }
        public double f64() { return Double.longBitsToDouble(u64()); }

        public String strValue() {
            if (u8() != T_STRING) throw new RuntimeException("wire: expected a string");
            return str();
        }

)JAVA" + str_switch_snippet(f) +
         R"JAVA(
    }
)JAVA" + encode_value_snippet(f) +
         "\n\n" + decode_value_snippet(f) + "\n" + value_to_string_snippet(f) + "\n" + list_helpers + R"JAVA(
    private S5RunWire() {}
}
)JAVA";
}

static std::string generate_schema_class(const schema_class &c) {
  std::string methods;
  for (const auto &m : c.methods) {
    std::string ret = j_is_void(m.return_type) ? "void" : type_to_java(m.return_type);
    std::string args;
    for (const auto &a : m.arguments) {
      if (!args.empty())
        args += ", ";
      args += type_to_java(a.type) + " " + a.name;
    }
    methods += "    " + ret + " " + m.name + "(" + args + ");\n";
  }
  std::string names;
  for (const auto &m : c.methods) {
    if (!names.empty())
      names += ", ";
    names += "\"" + m.name + "\"";
  }
  std::string props;
  for (const auto &p : c.properties) {
    if (!props.empty())
      props += ", ";
    props += "new S5RunProperty(\"" + p.name + "\", \"" + p.getter + "\", \"" + p.setter + "\")";
  }
  return "\npublic interface " + c.name +
         " extends S5RunObject {\n"
         "    @Override\n"
         "    default String s5runClassName() { return \"" +
         c.name +
         "\"; }\n"
         "\n"
         "    @Override\n"
         "    default List<S5RunProperty> s5runPropertyDescriptors() { return List.of(" +
         props +
         "); }\n"
         "\n"
         "    @Override\n"
         "    default List<String> s5runMethodNames() { return List.of(" +
         names +
         "); }\n"
         "\n" +
         methods + "}\n";
}

static std::string generate_schema_function(const schema_function &f) {
  std::string ret = j_is_void(f.return_type) ? "void" : type_to_java(f.return_type);
  std::string args;
  for (const auto &a : f.arguments) {
    if (!args.empty())
      args += ", ";
    args += type_to_java(a.type) + " " + a.name;
  }
  return "\npublic interface GlobalFunction_" + f.name +
         " extends S5RunFunction {\n"
         "    @Override\n"
         "    default String s5runFunctionName() { return \"" +
         f.name +
         "\"; }\n"
         "\n"
         "    " +
         ret + " execute(" + args +
         ");\n"
         "}\n";
}

static std::string generate_context(java_flavor f, const schema &s) {
  std::string method_handlers;
  for (const auto &c : s.classes) {
    for (const auto &m : c.methods) {
      if (!method_handlers.empty())
        method_handlers += "\n";
      method_handlers +=
        "                handlers.put(\"" + m.name + "\", (cbId, r) -> cbm_" + c.name + "_" + m.name + "(cbId, r));";
    }
  }

  std::string fn_callbacks;
  bool first_cb = true;
  for (const auto &f : s.functions) {
    j_method_like fm{f.name, f.return_type, f.arguments};
    std::string decode = decode_args(fm);
    std::string call = call_args(fm);
    std::string call_res, encode, head;
    if (j_is_void(fm.return_type)) {
      call_res = "as" + f.name + "().execute(" + call + ");";
      encode = "            w.u8(S5RunWire.T_UNDEFINED);";
    } else {
      call_res = "var res = as" + f.name + "().execute(" + call + ");";
      encode = encode_result(fm);
    }
    if (j_has_convert(f.arguments)) {
      head = "";
    } else {
      head = "            if (r.u32() != " + std::to_string(f.arguments.size()) + ") throw new RuntimeException(\"" +
             f.name + ": wrong argument count\");";
    }
    fn_callbacks += std::string(first_cb ? "\n" : "\n\n") + "    private void cb_" + f.name +
                    "(int cbId, S5RunWire.Reader r) throws Exception {\n"
                    "        " +
                    head +
                    "\n"
                    "        " +
                    decode +
                    "\n"
                    "        " +
                    call_res +
                    "\n"
                    "        S5RunWire.Writer w = new S5RunWire.Writer();\n"
                    "        w.i32(cbId);\n"
                    "        w.u8(1);\n" +
                    encode +
                    "\n"
                    "        sendMessage(S5RunWire.M_CALLBACK_RESP, w.toByteArray());\n"
                    "    }";
    first_cb = false;
  }

  std::string m_callbacks;
  for (const auto &c : s.classes) {
    for (const auto &m : c.methods) {
      j_method_like fm{c.name + "::" + m.name, m.return_type, m.arguments};
      std::string decode = decode_args(fm);
      std::string call = call_args(fm);
      bool conv = j_has_convert(m.arguments);
      std::string head;
      if (conv) {
        head = "            long n = r.u32();\n            n -= 1;";
      } else {
        head = "            if (r.u32() != " + std::to_string(1 + m.arguments.size()) +
               ") throw new RuntimeException(\"" + c.name + "." + m.name + ": wrong argument count\");";
      }
      std::string call_res, encode;
      if (j_is_void(m.return_type)) {
        call_res = "obj." + m.name + "(" + call + ");";
        encode = "            w.u8(S5RunWire.T_UNDEFINED);";
      } else {
        call_res = "var res = obj." + m.name + "(" + call + ");";
        encode = encode_result(fm);
      }
      if (conv) {
        const std::string &arg0 = m.arguments[0].name;
        decode = j_replace_all(decode, "long n_" + arg0 + " = r.u32();", "long n_ignored = 0;");
        // rewrite remaining n_<arg> references line by line
        std::string line, rewritten;
        for (size_t i = 0; i <= decode.size(); ++i) {
          if (i == decode.size() || decode[i] == '\n') {
            rewritten += j_replace_all(line, "n_" + arg0, "n") + "\n";
            line.clear();
          } else {
            line += decode[i];
          }
        }
        if (!rewritten.empty())
          rewritten.erase(rewritten.size() - 1);
        decode = rewritten;
      }
      m_callbacks += std::string(first_cb ? "\n" : "\n\n") + "    private void cbm_" + c.name + "_" + m.name +
                     "(int cbId, S5RunWire.Reader r) throws Exception {\n"
                     "        " +
                     head +
                     "\n"
                     "        int thisId = (int) r.readLongInt();\n"
                     "        " +
                     c.name + " obj = instanceById(\"" + c.name + "\", thisId);\n" + decode +
                     "\n"
                     "        " +
                     call_res +
                     "\n"
                     "        S5RunWire.Writer w = new S5RunWire.Writer();\n"
                     "        w.i32(cbId);\n"
                     "        w.u8(1);\n" +
                     encode +
                     "\n"
                     "        sendMessage(S5RunWire.M_CALLBACK_RESP, w.toByteArray());\n"
                     "    }";
      first_cb = false;
    }
  }

  std::string as_helpers;
  for (const auto &f : s.functions) {
    if (!as_helpers.empty())
      as_helpers += "\n";
    as_helpers += "    private GlobalFunction_" + f.name + " as" + f.name +
                  "() {\n"
                  "        return (GlobalFunction_" +
                  f.name + ") functions.get(\"" + f.name +
                  "\");\n"
                  "    }";
  }

  std::string cases;
  for (const auto &fn : s.functions) {
    if (!cases.empty())
      cases += "\n";
    cases += dispatch_case(f, fn.name);
  }

  std::string out = R"JAVA(import java.io.*;
import java.net.Socket;
import java.util.*;

public final class S5RunContext implements AutoCloseable {
    @FunctionalInterface
    interface ThrowingHandler {
        void handle(int cbId, S5RunWire.Reader r) throws Exception;
    }

    private final Map<String, ThrowingHandler> handlers = new HashMap<>();
    private final Map<String, S5RunFunction> functions = new HashMap<>();
    private final Map<String, Map<Integer, S5RunObject>> instances = new HashMap<>();
    private final Map<String, List<String>> knownClasses = new HashMap<>();
    private final List<S5RunRemoteServerInfo> servers = new ArrayList<>();
    private final List<PendingType> pendingTypes = new ArrayList<>();
    private final List<PendingObject> pendingObjects = new ArrayList<>();
    private final List<String> pendingFunctions = new ArrayList<>();
    private String script = "";
    private int nextThisId = 0;
    private Socket socket;
    private InputStream in;
    private OutputStream out;

)JAVA";
  out += pending_decls(f) + R"JAVA(

    public S5RunContext() {
)JAVA";
  out += method_handlers + R"JAVA(
    }

    public static String clientVersion() {
        return S5RunClientVersion.VERSION;
    }

    public S5RunContext setScript(String script) {
        this.script = script;
        return this;
    }

    public S5RunContext addServer(S5RunRemoteServerInfo server) {
        Objects.requireNonNull(server, "server");
        servers.add(server);
        return this;
    }

    public S5RunContext addClass(S5RunObject sample) {
        return addClass(sample.s5runClassName(), sample.s5runMethodNames(),
                sample.s5runPropertyDescriptors());
    }

    private S5RunContext addClass(String name, List<String> methods, List<S5RunProperty> props) {
        if (socket != null) {
            try {
                sendRegisterType(name, methods, props);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        } else {
            pendingTypes.add(new PendingType(name, methods, props));
        }
        return this;
    }

    public S5RunContext addObject(S5RunObject obj) {
        String cls = obj.s5runClassName();
        int id = ++nextThisId;
        instances.computeIfAbsent(cls, k -> new HashMap<>()).put(id, obj);
        if (socket != null) {
            try {
                sendRegisterObject(cls, id, obj.s5runMethodNames(), obj.s5runPropertyDescriptors());
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        } else {
            pendingObjects.add(new PendingObject(cls, id, obj.s5runMethodNames(),
                    obj.s5runPropertyDescriptors()));
        }
        return this;
    }

    public S5RunContext addFunction(S5RunFunction fn) {
        functions.put(fn.s5runFunctionName(), fn);
        String name = fn.s5runFunctionName();
        handlers.put(name, (cbId, r) -> dispatchByName(name, cbId, r));
        if (socket != null) {
            try {
                sendRegisterFunction(fn.s5runFunctionName());
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        } else {
            pendingFunctions.add(fn.s5runFunctionName());
        }
        return this;
    }

    // ------------------------------------------------------------------
    // public entry point
    // ------------------------------------------------------------------

    public static S5RunResult runScript(S5RunContext ctx, String entry, List<S5RunVariable> args) {
        long started = System.currentTimeMillis();
        try {
            ctx.connect();
            ctx.flushPending();
            S5RunWire.Writer w = new S5RunWire.Writer();
            w.str(ctx.script);
            w.str(entry);
            w.u32(args.size());
            for (S5RunVariable a : args) S5RunWire.encodeValue(w, a);
            ctx.sendMessage(S5RunWire.M_EXEC_REQ, w.toByteArray());
            while (true) {
                Frame f = ctx.readFrame();
                if (f.msgType == S5RunWire.M_CALLBACK_REQ) {
                    ctx.dispatchCallback(f.body);
                    continue;
                }
                if (f.msgType == S5RunWire.M_EXEC_RESP) {
                    S5RunWire.Reader r = new S5RunWire.Reader(f.body);
                    if (r.u8() == 0) {
                        throw new RuntimeException(r.str());
                    }
                    return S5RunResult.ok(S5RunWire.decodeValue(r), System.currentTimeMillis() - started);
                }
                throw new RuntimeException("s5run: unexpected message type");
            }
        } catch (Exception e) {
            return S5RunResult.fail(e.getMessage(), System.currentTimeMillis() - started);
        }
    }

    // ------------------------------------------------------------------
    // callbacks
    // ------------------------------------------------------------------

)JAVA";
  out += fn_callbacks;
  out += "\n";
  out += m_callbacks;
  out += "\n\n";
  out += as_helpers;
  out += R"JAVA(

    private void dispatchByName(String name, int cbId, S5RunWire.Reader r) throws Exception {
        switch (name) {
)JAVA";
  out += cases + "\n";
  out += dispatch_default(f) + R"JAVA(
        }
    }

    private void dispatchCallback(byte[] body) throws Exception {
        S5RunWire.Reader r = new S5RunWire.Reader(body);
        int cbId = r.i32();
        String method = r.str();
        ThrowingHandler handler = handlers.get(method);
        if (handler == null) {
            S5RunWire.Writer w = new S5RunWire.Writer();
            w.i32(cbId);
            w.u8(0);
            w.str("unknown method: " + method);
            sendMessage(S5RunWire.M_CALLBACK_RESP, w.toByteArray());
            return;
        }
        try {
            handler.handle(cbId, r);
        } catch (Exception e) {
            S5RunWire.Writer w = new S5RunWire.Writer();
            w.i32(cbId);
            w.u8(0);
            w.str(String.valueOf(e.getMessage()));
            sendMessage(S5RunWire.M_CALLBACK_RESP, w.toByteArray());
        }
    }

    @SuppressWarnings("unchecked")
    private <T extends S5RunObject> T instanceById(String cls, int thisId) {
        T obj = (T) instances.getOrDefault(cls, Map.of()).get(thisId);
        if (obj == null) {
            throw new RuntimeException(cls + ": unknown this_id " + thisId);
        }
        return obj;
    }

    private int registerInstance(S5RunObject obj) {
        int id = ++nextThisId;
        instances.computeIfAbsent(obj.s5runClassName(), k -> new HashMap<>()).put(id, obj);
        return id;
    }

    private int readRefId(S5RunWire.Reader r) {
        if (r.u8() != S5RunWire.T_REF) throw new RuntimeException("wire: expected an object reference");
        int id = r.i32();
        r.str(); // skip the type name
        return id;
    }

    private static String valueToString(S5RunVariable v) {
        return S5RunWire.valueToString(v);
    }

    // ------------------------------------------------------------------
    // connection
    // ------------------------------------------------------------------

    private void connect() throws Exception {
        if (socket != null) return;
        if (servers.isEmpty()) {
            throw new RuntimeException("s5run: no server added (addServer)");
        }
        int best = 0;
        if (servers.size() > 1) {
            long bestLoad = -1;
            for (int i = 0; i < servers.size(); i++) {
                long load = probeServerLoad(servers.get(i));
                if (load < 0) continue;
                if (bestLoad < 0 || load < bestLoad) {
                    bestLoad = load;
                    best = i;
                }
            }
            if (bestLoad < 0) throw new RuntimeException("s5run: all servers are unreachable");
        }
        S5RunRemoteServerInfo srv = servers.get(best);
        socket = new Socket(srv.getHost(), srv.getPort());
        socket.setTcpNoDelay(true);
        socket.setSoTimeout(10000);
        in = socket.getInputStream();
        out = socket.getOutputStream();
        readHello(in, out);
    }

    private static void readHello(InputStream in, OutputStream out) throws Exception {
        byte[] head = in.readNBytes(5);
        if (head.length < 5) throw new RuntimeException("s5run: expected server hello");
        int len = ((head[0] & 0xFF) << 24) | ((head[1] & 0xFF) << 16) | ((head[2] & 0xFF) << 8) | (head[3] & 0xFF);
        int msgType = head[4] & 0xFF;
        byte[] body = in.readNBytes(len - 1);
        if (msgType != S5RunWire.M_SERVER_HELLO || body.length < len - 1)
            throw new RuntimeException("s5run: expected server hello");
        String text = new String(body, java.nio.charset.StandardCharsets.UTF_8);
        String[] parts = text.split(" ");
        if (parts.length != 3 || !parts[0].equals("s5run"))
            throw new RuntimeException("s5run: bad server hello: " + text);
        int protocol = Integer.parseInt(parts[2]);
        if (protocol != 2)
            throw new RuntimeException("s5run: server protocol version " + protocol
                    + " is not supported (client supports 2)");
    }

    private static long probeServerLoad(S5RunRemoteServerInfo server) {
        try (Socket probe = new Socket()) {
            probe.connect(new java.net.InetSocketAddress(server.getHost(), server.getPort()), 2000);
            probe.setTcpNoDelay(true);
            probe.setSoTimeout(2000);
            OutputStream po = probe.getOutputStream();
            InputStream pi = probe.getInputStream();
            readHello(pi, po);
            writeFrame(po, S5RunWire.M_STATS_REQ, new byte[0]);
            byte[] head = pi.readNBytes(5);
            if (head.length < 5) return -1;
            int len = ((head[0] & 0xFF) << 24) | ((head[1] & 0xFF) << 16) | ((head[2] & 0xFF) << 8) | (head[3] & 0xFF);
            int msgType = head[4] & 0xFF;
            byte[] body = pi.readNBytes(len - 1);
            if (msgType != S5RunWire.M_STATS_RESP || body.length < 16) return -1;
            S5RunWire.Reader r = new S5RunWire.Reader(body);
            r.u32(); // threads
            long busy = r.u32();
            long queued = r.u32();
            return busy + queued;
        } catch (Exception e) {
            return -1;
        }
    }

    private void flushPending() throws Exception {
)JAVA";
  out += flush_pending_body(f) + R"JAVA(
        pendingObjects.clear();
        for (String name : new ArrayList<>(pendingFunctions)) {
            sendRegisterFunction(name);
        }
        pendingFunctions.clear();
    }

    private static void putPropertyDescriptors(S5RunWire.Writer w, List<S5RunProperty> props) {
        w.u32(props.size());
        for (S5RunProperty p : props) {
            w.str(p.name);
            w.str(p.getter);
            w.str(p.setter);
        }
    }

    private void sendRegisterType(String name, List<String> methods,
                                  List<S5RunProperty> props) throws Exception {
        S5RunWire.Writer w = new S5RunWire.Writer();
        w.str(name);
        w.u32(methods.size());
        for (String m : methods) w.str(m);
        putPropertyDescriptors(w, props);
        sendMessage(S5RunWire.M_REGISTER_TYPE, w.toByteArray());
        expectRegisterResp("register_type(" + name + ")");
    }

    private void sendRegisterObject(String name, int thisId, List<String> methods,
                                    List<S5RunProperty> props) throws Exception {
        S5RunWire.Writer w = new S5RunWire.Writer();
        w.str(name);
        w.i32(thisId);
        w.str(name);
        w.u32(methods.size());
        for (String m : methods) w.str(m);
        putPropertyDescriptors(w, props);
        sendMessage(S5RunWire.M_REGISTER_OBJ, w.toByteArray());
        expectRegisterResp("register_object(" + name + ")");
    }

    private void sendRegisterFunction(String name) throws Exception {
        S5RunWire.Writer w = new S5RunWire.Writer();
        w.str(name);
        sendMessage(S5RunWire.M_REGISTER_FUNC, w.toByteArray());
        expectRegisterResp("register_function(" + name + ")");
    }

    private void expectRegisterResp(String what) throws Exception {
        Frame f = readFrame();
        if (f.msgType != S5RunWire.M_REGISTER_RESP) {
            throw new RuntimeException("s5run: " + what + " failed");
        }
        S5RunWire.Reader r = new S5RunWire.Reader(f.body);
        if (r.u8() == 0) {
            throw new RuntimeException("s5run: " + what + " error: " + r.str());
        }
    }

    /** Closes the connection; the context can be reused afterwards. */
    @Override
    public void close() {
        if (socket != null) {
            try {
                socket.close();
            } catch (IOException ignored) {
            }
            socket = null;
            in = null;
            out = null;
        }
    }

    // ------------------------------------------------------------------
    // framing
    // ------------------------------------------------------------------

)JAVA";
  out += frame_decl(f) + R"JAVA(

    private void sendMessage(int msgType, byte[] body) throws Exception {
        writeFrame(out, msgType, body);
    }

    private static void writeFrame(OutputStream os, int msgType, byte[] body) throws Exception {
        int len = 1 + body.length;
        os.write((len >> 24) & 0xFF);
        os.write((len >> 16) & 0xFF);
        os.write((len >> 8) & 0xFF);
        os.write(len & 0xFF);
        os.write(msgType);
        os.write(body);
        os.flush();
    }

    private Frame readFrame() throws Exception {
        byte[] head = in.readNBytes(5);
        if (head.length < 5) throw new RuntimeException("s5run: connection closed");
        int len = ((head[0] & 0xFF) << 24) | ((head[1] & 0xFF) << 16) | ((head[2] & 0xFF) << 8) | (head[3] & 0xFF);
        if (len <= 0 || len > 64 * 1024 * 1024) throw new RuntimeException("s5run: bad frame length");
        int msgType = head[4] & 0xFF;
        byte[] body = in.readNBytes(len - 1);
        if (body.length < len - 1) throw new RuntimeException("s5run: connection closed");
        return new Frame(msgType, body);
    }
}
)JAVA";
  return out;
}

// -----------------------------------------------------------------------
// Generator
// -----------------------------------------------------------------------

class generator_java : public IGenerator {
public:
  generator_java(const std::string &java_xx) : m_java_xx(java_xx) {
    if (m_java_xx == "java11") {
      m_flavor = JAVA_11;
    } else if (m_java_xx == "java17") {
      m_flavor = JAVA_17;
    } else if (m_java_xx == "java21") {
      m_flavor = JAVA_21;
    }
  }

  std::vector<std::string> generate_client(const schema &s, const std::string &output_dir) override;
  std::vector<std::string> generate_provider(const schema &s, const std::string &output_dir) override;

private:
  java_flavor m_flavor;
  std::string m_java_xx;
};

static std::string trim(const std::string &s) {
  size_t b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos)
    return "";
  size_t e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}

static void mkdir_p(const std::string &path) {
  std::string cur;
  for (size_t i = 0; i < path.size(); ++i) {
    cur += path[i];
    if (path[i] == '/' && i > 0)
      ::mkdir(cur.substr(0, i).c_str(), 0755);
  }
  ::mkdir(path.c_str(), 0755);
}

static void write_java_file(
  const std::string &pkg_dir,
  const std::string &pkg,
  const std::string &name,
  const std::string &body,
  std::vector<std::string> &written
) {
  std::string path = pkg_dir + "/" + name + ".java";
  std::ofstream f(path.c_str());
  f << "// AUTO-GENERATED. DO NOT EDIT.\n";
  f << "package " << pkg << ";\n\n";
  if (body.find("List<") != std::string::npos || body.find("Map<") != std::string::npos)
    f << "import java.util.*;\n\n";
  f << trim(body) << "\n";
  f.close();
  written.push_back(path);
}

std::vector<std::string> generator_java::generate_client(const schema &s, const std::string &output_dir) {
  auto options = s.options_java(m_java_xx);

  std::vector<std::string> written;
  std::string pkg = options->get_package_name();

  std::string pkg_dir = output_dir;
  if (!pkg.empty()) {
    std::string rel = pkg;
    for (auto &c : rel)
      if (c == '.')
        c = '/';
    pkg_dir += "/" + rel;
  }
  mkdir_p(pkg_dir);

  write_java_file(pkg_dir, pkg, "S5RunWire", generate_wire(m_flavor, s), written);
  write_java_file(pkg_dir, pkg, "S5RunVariable", value_file_snippet(m_flavor), written);
  write_java_file(
    pkg_dir,
    pkg,
    "S5RunProperty",
    "\npublic final class S5RunProperty {\n"
    "    public final String name;\n"
    "    public final String getter;\n"
    "    public final String setter;\n"
    "\n"
    "    public S5RunProperty(String name, String getter, String setter) {\n"
    "        this.name = name;\n"
    "        this.getter = getter;\n"
    "        this.setter = setter;\n"
    "    }\n"
    "}\n",
    written
  );
  write_java_file(
    pkg_dir,
    pkg,
    "S5RunObject",
    "\npublic interface S5RunObject {\n"
    "    String s5runClassName();\n"
    "    List<String> s5runMethodNames();\n"
    "\n"
    "    /** (name, getter, setter) descriptors; JS: obj.<name> read/write. */\n"
    "    default List<S5RunProperty> s5runPropertyDescriptors() { return List.of(); }\n"
    "}\n",
    written
  );
  write_java_file(
    pkg_dir,
    pkg,
    "S5RunFunction",
    "\npublic interface S5RunFunction {\n"
    "    String s5runFunctionName();\n"
    "}\n",
    written
  );
  for (const auto &c : s.classes)
    write_java_file(pkg_dir, pkg, c.name, generate_schema_class(c), written);
  for (const auto &f : s.functions)
    write_java_file(pkg_dir, pkg, "GlobalFunction_" + f.name, generate_schema_function(f), written);
  write_java_file(
    pkg_dir,
    pkg,
    "S5RunRemoteServerInfo",
    "\npublic final class S5RunRemoteServerInfo {\n"
    "    private String address = \"\";\n"
    "    private String apiKey = \"\";\n"
    "    private String host = \"\";\n"
    "    private int port;\n"
    "\n"
    "    public S5RunRemoteServerInfo address(String address) {\n"
    "        final String prefix = \"s5run://\";\n"
    "        if (!address.startsWith(prefix)) {\n"
    "            throw new IllegalArgumentException(\"expected format s5run://host:port\");\n"
    "        }\n"
    "        String rest = address.substring(prefix.length());\n"
    "        int colon = rest.lastIndexOf(':');\n"
    "        if (colon <= 0 || colon + 1 >= rest.length()) {\n"
    "            throw new IllegalArgumentException(\"expected format s5run://host:port\");\n"
    "        }\n"
    "        this.address = address;\n"
    "        this.host = rest.substring(0, colon);\n"
    "        this.port = Integer.parseInt(rest.substring(colon + 1));\n"
    "        return this;\n"
    "    }\n"
    "\n"
    "    public S5RunRemoteServerInfo apiKey(String apiKey) {\n"
    "        this.apiKey = apiKey;\n"
    "        return this;\n"
    "    }\n"
    "\n"
    "    public String getAddress() { return address; }\n"
    "    public String getApiKey() { return apiKey; }\n"
    "    public String getHost() { return host; }\n"
    "    public int getPort() { return port; }\n"
    "}\n",
    written
  );
  write_java_file(pkg_dir, pkg, "S5RunResult", run_result_file_snippet(m_flavor), written);
  write_java_file(pkg_dir, pkg, "S5RunContext", generate_context(m_flavor, s), written);
  write_java_file(
    pkg_dir,
    pkg,
    "S5RunClientVersion",
    "\npublic final class S5RunClientVersion {\n"
    "    public static final String VERSION = \"" +
      s.client_version +
      "\";\n"
      "\n"
      "    public static String get() {\n"
      "        return VERSION;\n"
      "    }\n"
      "    private S5RunClientVersion() {}\n"
      "}\n",
    written
  );
  return written;
}

std::vector<std::string> generator_java::generate_provider(const schema &s, const std::string &output_dir) {
  return {};
}

std::shared_ptr<IGenerator> make_generator_java11() {
  return std::make_shared<generator_java>("java11");
}

std::shared_ptr<IGenerator> make_generator_java17() {
  return std::make_shared<generator_java>("java17");
}

std::shared_ptr<IGenerator> make_generator_java21() {
  return std::make_shared<generator_java>("java21");
}

} // namespace generators
} // namespace s5run
