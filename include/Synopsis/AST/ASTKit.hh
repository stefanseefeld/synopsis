//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_ASTKit_hh
#define _Synopsis_AST_ASTKit_hh

#include <Synopsis/Kit.hh>
#include <Synopsis/AST/AST.hh>
#include <Synopsis/AST/SourceFile.hh>
#include <Synopsis/AST/Declaration.hh>

namespace Synopsis
{
namespace AST
{

// basically a factory for all AST types
class ASTKit : public Kit
{
public:
  ASTKit() : Kit("Synopsis.AST") {}

  AST create_ast() { return create<AST>("AST");}

  Comment create_comment(const SourceFile &file, long line,
			 const std::string &text, bool suspect=false)
  { return create<Comment>("Declaration", Tuple(text, file, line, suspect));}
  
  Declaration create_declaration(const SourceFile &sf, long line, const std::string &lang,
				 const char *type, const ScopedName &name)
  { return create<Declaration>("Declaration", Tuple(sf, line, lang, type, name));}

  Builtin create_builtin(const SourceFile &file, int line, const std::string &lang,
			 const std::string &type, const ScopedName &name)
  { return create<Builtin>("Builtin", Tuple(file, line, lang, type, name));}

  Include create_include(const SourceFile &sf, bool is_macro, bool is_next)
  { return create<Include>("Include", Tuple(sf, is_macro, is_next));}

  Macro create_macro(SourceFile &sf, long line, const std::string &lang,
		     const ScopedName &name, const List &parameters,
		     const std::string &text)
  { return create<Macro>("Macro", Tuple(sf, line, lang, "macro",
					name, parameters, text));}

  Forward create_forward(const SourceFile &file, int line, const std::string &lang,
			 const std::string &type, const ScopedName &name)
  { return create<Forward>("Forward", Tuple(file, line, lang, type, name));}

  MacroCall create_macro_call(const std::string &name, int start, int end, int diff)
  { return create<MacroCall>("MacroCall", Tuple(name, start, end, diff));}

  Scope create_scope(const SourceFile &file, int line, const std::string &lang,
		     const std::string &type, const ScopedName &name)
  { return create<Scope>("Scope", Tuple(file, line, lang, type, name));}

  Synopsis::AST::Module
  create_module(const SourceFile &file, int line, const std::string &lang,
		const std::string &type, const ScopedName &name)
  { return create<Synopsis::AST::Module>("Module", Tuple(file, line, lang, type, name));}

  Inheritance create_inheritance(const Type &parent,
				 const List &attributes)
  { return create<Inheritance>("Inheritance", Tuple(parent, attributes));}

  Class create_class(const SourceFile &file, int line, const std::string &lang,
		     const std::string &type, const ScopedName &name)
  { return create<Class>("Class", Tuple(file, line, lang, type, name));}

  Typedef create_typedef(const SourceFile &file, int line, const std::string &lang,
			 const std::string &type, const ScopedName &name,
			 const Type &alias, bool constr)
  { return create<Typedef>("Typedef", Tuple(file, line, lang, type, name, alias, constr));}

  Enumerator create_enumerator(const SourceFile &file, int line, const std::string &lang,
			       const std::string &type, const ScopedName &name,
			       const std::string &value)
  { return create<Enumerator>("Enumerator", Tuple(file, line, lang, type, name, value));}

  Enum create_enum(const SourceFile &file, int line, const std::string &lang,
		   const std::string &type, const ScopedName &name)
  { return create<Enum>("Enum", Tuple(file, line, lang, type, name));}

  Variable create_variable(const SourceFile &file, int line, const std::string &lang,
			   const std::string &type, const ScopedName &name,
			   const Type &vtype, bool constr)
  { return create<Variable>("Variable", Tuple(file, line, lang, type, name, vtype, constr));}

  Const create_const(const SourceFile &file, int line, const std::string &lang,
		     const std::string &type, const ScopedName &name,
		     const Type &ctype, const std::string &value)
  { return create<Const>("Const", Tuple(file, line, lang, type, name, ctype, value));}

  Parameter create_parameter(const Modifiers &pre, const Type &type, const Modifiers &post,
			     const std::string &name, const std::string &value)
  { return create<Parameter>("Parameter", Tuple(pre, type, post, name, value));}

  Function create_function(const SourceFile &file, int line, const std::string &lang,
 			   const std::string &type, const Modifiers &pre,
 			   const Type &ret, const ScopedName &name,
 			   const std::string &realname)
  { return create<Function>("Function", Tuple(file, line, lang, type, pre, ret,
 					      name, realname));}

  Operation create_operation(const SourceFile &file, int line, const std::string &lang,
			     const std::string &type, const ScopedName &name,
			     const Modifiers &pre, const Type &ret,
			     const std::string &realname)
  { return create<Operation>("Operation", Tuple(file, line, lang, type, name,
						pre, ret, realname));}

  SourceFile create_source_file(const std::string &name,
				const std::string &longname,
				const std::string &lang)
  { return create<SourceFile>("SourceFile", Tuple(name, longname, lang));}

};

}
}

#endif
