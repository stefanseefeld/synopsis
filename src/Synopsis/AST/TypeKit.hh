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
  TypeKit(std::string const &lang) : Kit("Synopsis.Type"), my_lang(lang) {}

  Type create_type()
  { return create<Type>("Type", Python::Tuple(my_lang));}

  Named create_named(const ScopedName &sn)
  { return create<Named>("Named", Python::Tuple(my_lang, sn));}

  Base create_base(const ScopedName &sn)
  { return create<Base>("Base", Python::Tuple(my_lang, sn));}

  Dependent create_dependent(const ScopedName &sn)
  { return create<Dependent>("Dependent", Python::Tuple(my_lang, sn));}

  Unknown create_unknown(const ScopedName &sn)
  { return create<Unknown>("Unknown", Python::Tuple(my_lang, sn));}

  Declared create_declared(const ScopedName &sn,
			   const Declaration &decl)
  { return create<Declared>("Declared", Python::Tuple(my_lang, sn, decl));}

  Template create_template(const ScopedName &sn,
			   const Declaration &decl, const Python::List &params)
  { return create<Template>("Template", Python::Tuple(my_lang, sn, decl, params));}

  Modifier create_modifier(const Type &alias,
			   const Modifiers &pre, const Modifiers &post)
  { return create<Modifier>("Modifier", Python::Tuple(my_lang, alias, pre, post));}

  Array create_array(const Type &alias,
		     const Python::TypedList<int> &sizes)
  { return create<Array>("Array", Python::Tuple(my_lang, alias, sizes));}

  Parametrized create_parametrized(const Template &t,
				   const Python::List &params)
  { return create<Parametrized>("Parametrized", Python::Tuple(my_lang, t, params));}

  FunctionPtr create_function_ptr(const Type &retn,
				  const Modifiers &pre, const TypeList &params)
  { return create<FunctionPtr>("Function", Python::Tuple(my_lang, retn, pre, params));}

private:
  std::string my_lang;
};

}
}

#endif
