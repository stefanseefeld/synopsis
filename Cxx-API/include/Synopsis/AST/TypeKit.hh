//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_TypeKit_hh
#define _Synopsis_AST_TypeKit_hh

#include <Synopsis/Python/Kit.hh>
#include <Synopsis/AST/Type.hh>
#include <Synopsis/AST/Declared.hh>

namespace Synopsis
{
namespace AST
{

// basically a factory for all Type types
class TypeKit : public Python::Kit
{
public:
  TypeKit() : Kit("Synopsis.Type") {}

  Type create_type(const std::string &lang)
  { return create<Type>("Type", Python::Tuple(lang));}

  Named create_named(const std::string &lang, const ScopedName &sn)
  { return create<Named>("Named", Python::Tuple(lang, sn));}

  Base create_base(const std::string &lang, const ScopedName &sn)
  { return create<Base>("Base", Python::Tuple(lang, sn));}

  Dependent create_dependent(const std::string &lang, const ScopedName &sn)
  { return create<Dependent>("Dependent", Python::Tuple(lang, sn));}

  Unknown create_unknown(const std::string &lang, const ScopedName &sn)
  { return create<Unknown>("Unknown", Python::Tuple(lang, sn));}

  Declared create_declared(const std::string &lang, const ScopedName &sn,
			   const Declaration &decl)
  { return create<Declared>("Declared", Python::Tuple(lang, sn, decl));}

  Template create_template(const std::string &lang, const ScopedName &sn,
			   const Declaration &decl, const Python::List &params)
  { return create<Template>("Template", Python::Tuple(lang, sn, decl, params));}

  Modifier create_modifier(const std::string &lang, const Type &alias,
			   const Modifiers &pre, const Modifiers &post)
  { return create<Modifier>("Modifier", Python::Tuple(lang, alias, pre, post));}

  Array create_array(const std::string &lang, const Type &alias,
		     const Python::TypedList<int> &sizes)
  { return create<Array>("Array", Python::Tuple(lang, alias, sizes));}

  Parametrized create_parametrized(const std::string &lang, const Template &t,
				   const Python::List &params)
  { return create<Parametrized>("Parametrized", Python::Tuple(lang, t, params));}

  FunctionPtr create_function_ptr(const std::string &lang, const Type &retn,
				  const Modifiers &pre, const TypeList &params)
  { return create<FunctionPtr>("Function", Python::Tuple(lang, retn, pre, params));}

};

}
}

#endif
