//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_ASG_ASGKit_hh
#define _Synopsis_ASG_ASGKit_hh

#include <Synopsis/Python/Kit.hh>
#include <Synopsis/ASG/IR.hh>
#include <Synopsis/ASG/Type.hh>
#include <Synopsis/ASG/SourceFile.hh>
#include <Synopsis/ASG/Declaration.hh>

namespace Synopsis
{
class IRKit : public Python::Kit
{
public:
  IRKit() : Python::Kit("Synopsis.IR") {}

  IR create_ir() { return create<IR>("IR");}
};

class QNameKit : public Python::Kit
{
public:
  QNameKit() : Python::Kit("Synopsis.QualifiedName") {}
  Object create_qname(ScopedName const &name)
  { return create<Object>("QualifiedCxxName", Python::Tuple(name));}
};

namespace ASG
{
// basically a factory for all ASG types
class ASGKit : public Python::Kit
{
public:
  ASGKit() : Python::Kit("Synopsis.ASG") {}

  Declaration create_declaration(const SourceFile &sf, long line,
				 const char *type, const ScopedName &name)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Declaration>("Declaration", Python::Tuple(sf, line, type, qname));
  }

  Builtin create_builtin(const SourceFile &file, int line,
			 const std::string &type, const ScopedName &name)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Builtin>("Builtin", Python::Tuple(file, line, type, qname));
  }

  Macro create_macro(SourceFile &sf, long line,
		     const ScopedName &name, const Python::List &parameters,
		     const std::string &text)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Macro>("Macro", Python::Tuple(sf, line, "macro",
					qname, parameters, text));
  }

  Forward create_forward(const SourceFile &file, int line,
			 const std::string &type, const ScopedName &name)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Forward>("Forward", Python::Tuple(file, line, type, qname));
  }

  Scope create_scope(const SourceFile &file, int line,
		     const std::string &type, const ScopedName &name)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Scope>("Scope", Python::Tuple(file, line, type, qname));
  }

  Synopsis::ASG::Module
  create_module(const SourceFile &file, int line,
		const std::string &type, const ScopedName &name)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Synopsis::ASG::Module>("Module", Python::Tuple(file, line, type, qname));
  }

  Inheritance create_inheritance(const Type &parent,
				 const Python::List &attributes)
  { return create<Inheritance>("Inheritance", Python::Tuple(parent, attributes));}

  Class create_class(const SourceFile &file, int line,
		     const std::string &type, const ScopedName &name)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Class>("Class", Python::Tuple(file, line, type, qname));
  }

  Typedef create_typedef(const SourceFile &file, int line,
			 const std::string &type, const ScopedName &name,
			 const Type &alias, bool constr)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Typedef>("Typedef", Python::Tuple(file, line, type, qname, alias, constr));
  }

  Enumerator create_enumerator(const SourceFile &file, int line,
			       const ScopedName &name, const std::string &value)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Enumerator>("Enumerator", Python::Tuple(file, line, qname, value));
  }

  Enum create_enum(const SourceFile &file, int line,
		   const ScopedName &name, const Enumerators &values)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Enum>("Enum", Python::Tuple(file, line, qname, values));
  }

  Variable create_variable(const SourceFile &file, int line,
			   const std::string &type, const ScopedName &name,
			   const Type &vtype, bool constr)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Variable>("Variable", Python::Tuple(file, line, type, qname, vtype, constr));
  }

  Const create_const(const SourceFile &file, int line,
		     const std::string &type, const ScopedName &name,
		     const Type &ctype, const std::string &value)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Const>("Const", Python::Tuple(file, line, type, qname, ctype, value));
  }

  Parameter create_parameter(const Modifiers &pre, const Type &type, const Modifiers &post,
			     const std::string &name, const std::string &value)
  { return create<Parameter>("Parameter", Python::Tuple(pre, type, post, name, value));}

  Function create_function(const SourceFile &file, int line,
 			   const std::string &type, const Modifiers &pre,
 			   const Type &ret, const Modifiers &post,
			   const ScopedName &name, const std::string &realname)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Function>("Function", Python::Tuple(file, line, type, pre, ret, post,
						      qname, realname));
  }

  Operation create_operation(const SourceFile &file, int line,
			     const std::string &type, const Modifiers &pre,
			     const Type &ret, const Modifiers &post,
			     const ScopedName &name, const std::string &realname)
  {
    Python::Object qname = qname_kit_.create_qname(name);
    return create<Operation>("Operation", Python::Tuple(file, line, type, pre, ret, post,
							qname, realname));
  }
private:
  QNameKit qname_kit_;

};
}

class SourceFileKit : public Python::Kit
{
public:
  SourceFileKit(std::string const &lang) : Python::Kit("Synopsis.SourceFile"), lang_(lang) {}
  SourceFile create_source_file(const std::string &name,
				const std::string &longname)
  { return create<SourceFile>("SourceFile", Python::Tuple(name, longname, lang_));}

  Include create_include(const SourceFile &sf, const std::string &name,
			 bool is_macro, bool is_next)
  { return create<Include>("Include", Python::Tuple(sf, name, is_macro, is_next));}

  MacroCall create_macro_call(const std::string &name,
                              int start_line, int start_col, int end_line, int end_col,
                              int e_start_line, int e_start_col, int e_end_line, int e_end_col)
  { return create<MacroCall>("MacroCall",
                             Python::Tuple(name,
                                           Python::Tuple(start_line, start_col),
                                           Python::Tuple(end_line, end_col),
                                           Python::Tuple(e_start_line, e_start_col),
                                           Python::Tuple(e_end_line, e_end_col)));}
private:
  std::string lang_;
};

}

#endif
