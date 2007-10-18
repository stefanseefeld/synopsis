//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Synopsis_ASG_TypeKit_hh
#define _Synopsis_ASG_TypeKit_hh

#include <Synopsis/Python/Kit.hh>
#include <Synopsis/ASG/Type.hh>
#include <Synopsis/ASG/Declared.hh>

namespace Synopsis
{
namespace ASG
{

// basically a factory for all Type types
class TypeKit : public Python::Kit
{
public:
  TypeKit(std::string const &lang) : Kit("Synopsis.Type"), lang_(lang) {}

  Type create_type()
  { return create<Type>("Type", Python::Tuple(lang_));}

  Named create_named(const ScopedName &sn)
  { return create<Named>("Named", Python::Tuple(lang_, sn));}

  Base create_base(const ScopedName &sn)
  { return create<Base>("Base", Python::Tuple(lang_, sn));}

  Dependent create_dependent(const ScopedName &sn)
  { return create<Dependent>("Dependent", Python::Tuple(lang_, sn));}

  Unknown create_unknown(const ScopedName &sn)
  { return create<Unknown>("Unknown", Python::Tuple(lang_, sn));}

  Declared create_declared(const ScopedName &sn,
			   const Declaration &decl)
  { return create<Declared>("Declared", Python::Tuple(lang_, sn, decl));}

  Template create_template(const ScopedName &sn,
			   const Declaration &decl, const Python::List &params)
  { return create<Template>("Template", Python::Tuple(lang_, sn, decl, params));}

  Modifier create_modifier(const Type &alias,
			   const Modifiers &pre, const Modifiers &post)
  { return create<Modifier>("Modifier", Python::Tuple(lang_, alias, pre, post));}

  Array create_array(const Type &alias,
		     const Python::TypedList<int> &sizes)
  { return create<Array>("Array", Python::Tuple(lang_, alias, sizes));}

  Parametrized create_parametrized(const Template &t,
				   const Python::List &params)
  { return create<Parametrized>("Parametrized", Python::Tuple(lang_, t, params));}

  FunctionPtr create_function_ptr(const Type &retn,
				  const Modifiers &pre, const TypeList &params)
  { return create<FunctionPtr>("Function", Python::Tuple(lang_, retn, pre, params));}

private:
  std::string lang_;
};

}
}

#endif
