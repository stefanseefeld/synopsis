//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_ASTKit_hh
#define _Synopsis_AST_ASTKit_hh

#include <Synopsis/Python/Kit.hh>
#include <Synopsis/AST/AST.hh>
#include <Synopsis/AST/SourceFile.hh>
#include <Synopsis/AST/Declaration.hh>

namespace Synopsis
{
namespace AST
{

// basically a factory for all AST types
class ASTKit : public Python::Kit
{
public:
  ASTKit(std::string const &lang) : Python::Kit("Synopsis.AST"), my_lang(lang) {}

  AST create_ast() { return create<AST>("AST");}

  Comment create_comment(const SourceFile &file, long line,
			 const std::string &text, bool suspect=false)
  { return create<Comment>("Comment", Python::Tuple(text, file, line, suspect));}
  
  Declaration create_declaration(const SourceFile &sf, long line,
				 const char *type, const ScopedName &name)
  { return create<Declaration>("Declaration", Python::Tuple(sf, line, my_lang, type, name));}

  Builtin create_builtin(const SourceFile &file, int line,
			 const std::string &type, const ScopedName &name)
  { return create<Builtin>("Builtin", Python::Tuple(file, line, my_lang, type, name));}

  Include create_include(const SourceFile &sf, const std::string &name,
			 bool is_macro, bool is_next)
  { return create<Include>("Include", Python::Tuple(sf, name, is_macro, is_next));}

  Macro create_macro(SourceFile &sf, long line,
		     const ScopedName &name, const Python::List &parameters,
		     const std::string &text)
  { return create<Macro>("Macro", Python::Tuple(sf, line, my_lang, "macro",
					name, parameters, text));}

  Forward create_forward(const SourceFile &file, int line,
			 const std::string &type, const ScopedName &name)
  { return create<Forward>("Forward", Python::Tuple(file, line, my_lang, type, name));}

  MacroCall create_macro_call(const std::string &name, int start, int end, int diff)
  { return create<MacroCall>("MacroCall", Python::Tuple(name, start, end, diff));}

  Scope create_scope(const SourceFile &file, int line,
		     const std::string &type, const ScopedName &name)
  { return create<Scope>("Scope", Python::Tuple(file, line, my_lang, type, name));}

  Synopsis::AST::Module
  create_module(const SourceFile &file, int line,
		const std::string &type, const ScopedName &name)
  { return create<Synopsis::AST::Module>("Module", Python::Tuple(file, line, my_lang, type, name));}

  Inheritance create_inheritance(const Type &parent,
				 const Python::List &attributes)
  { return create<Inheritance>("Inheritance", Python::Tuple(parent, attributes));}

  Class create_class(const SourceFile &file, int line,
		     const std::string &type, const ScopedName &name)
  { return create<Class>("Class", Python::Tuple(file, line, my_lang, type, name));}

  Typedef create_typedef(const SourceFile &file, int line,
			 const std::string &type, const ScopedName &name,
			 const Type &alias, bool constr)
  { return create<Typedef>("Typedef", Python::Tuple(file, line, my_lang, type, name, alias, constr));}

  Enumerator create_enumerator(const SourceFile &file, int line,
			       const ScopedName &name, const std::string &value)
  { return create<Enumerator>("Enumerator", Python::Tuple(file, line, my_lang, name, value));}

  Enum create_enum(const SourceFile &file, int line,
		   const ScopedName &name, const Enumerators &values)
  { return create<Enum>("Enum", Python::Tuple(file, line, my_lang, name, values));}

  Variable create_variable(const SourceFile &file, int line,
			   const std::string &type, const ScopedName &name,
			   const Type &vtype, bool constr)
  { return create<Variable>("Variable", Python::Tuple(file, line, my_lang, type, name, vtype, constr));}

  Const create_const(const SourceFile &file, int line,
		     const std::string &type, const ScopedName &name,
		     const Type &ctype, const std::string &value)
  { return create<Const>("Const", Python::Tuple(file, line, my_lang, type, name, ctype, value));}

  Parameter create_parameter(const Modifiers &pre, const Type &type, const Modifiers &post,
			     const std::string &name, const std::string &value)
  { return create<Parameter>("Parameter", Python::Tuple(pre, type, post, name, value));}

  Function create_function(const SourceFile &file, int line,
 			   const std::string &type, const Modifiers &pre,
 			   const Type &ret, const Modifiers &post,
			   const ScopedName &name, const std::string &realname)
  { return create<Function>("Function", Python::Tuple(file, line, my_lang, type, pre, ret, post,
						      name, realname));}

  Operation create_operation(const SourceFile &file, int line,
			     const std::string &type, const Modifiers &pre,
			     const Type &ret, const Modifiers &post,
			     const ScopedName &name, const std::string &realname)
  { return create<Operation>("Operation", Python::Tuple(file, line, my_lang, type, pre, ret, post,
							name, realname));}

  SourceFile create_source_file(const std::string &name,
				const std::string &longname)
  { return create<SourceFile>("SourceFile", Python::Tuple(name, longname, my_lang));}
private:
  std::string my_lang;
};

}
}

#endif
