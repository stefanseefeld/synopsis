// $Id: TypeKit.hh,v 1.1 2004/01/25 21:21:54 stefan Exp $
//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_AST_TypeKit_hh
#define _Synopsis_AST_TypeKit_hh

#include <Synopsis/Kit.hh>
#include <Synopsis/AST/Type.hh>
#include <Synopsis/AST/Declared.hh>

namespace Synopsis
{
namespace AST
{

// basically a factory for all Type types
class TypeKit : public Kit
{
public:
  TypeKit() : Kit("Synopsis.Type") {}

  Type create_type(const std::string &lang)
  { return create<Type>("Type", Tuple(lang));}

  Named create_named(const std::string &lang, const ScopedName &sn)
  { return create<Named>("Named", Tuple(lang, sn));}

  Base create_base(const std::string &lang, const ScopedName &sn)
  { return create<Base>("Base", Tuple(lang, sn));}

  Dependent create_dependent(const std::string &lang, const ScopedName &sn)
  { return create<Dependent>("Dependent", Tuple(lang, sn));}

  Unknown create_unknown(const std::string &lang, const ScopedName &sn)
  { return create<Unknown>("Unknown", Tuple(lang, sn));}

  Declared create_declared(const std::string &lang, const ScopedName &sn,
			   const Declaration &decl)
  { return create<Declared>("Declared", Tuple(lang, sn, decl));}

  Template create_template(const std::string &lang, const ScopedName &sn,
			   const Declaration &decl, const List &params)
  { return create<Template>("Template", Tuple(lang, sn, decl, params));}

  Modifier create_modifier(const std::string &lang, const Type &alias,
			   const std::string &pre, const std::string &post)
  { return create<Modifier>("Modifier", Tuple(lang, alias, pre, post));}

  Array create_array(const std::string &lang, const Type &alias,
		     const TypedList<size_t> &sizes)
  { return create<Array>("Array", Tuple(lang, alias, sizes));}

  Parametrized create_parametrized(const std::string &lang, const Template &t,
				   const List &params)
  { return create<Parametrized>("Parametrized", Tuple(lang, t, params));}

  FunctionPtr create_function_ptr(const std::string &lang, const Type &retn,
				  const Modifiers &pre, const TypeList &params)
  { return create<FunctionPtr>("Function", Tuple(lang, retn, pre, params));}

};

}
}

#endif
