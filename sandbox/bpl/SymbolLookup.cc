//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/SymbolLookup.hh>
#include <boost/python.hpp>

namespace bpl = boost::python;
using namespace Synopsis;
using namespace Synopsis::SymbolLookup;

namespace
{

SymbolSet lookup_symbol(Scope const *scope, PTree::Encoding const &enc)
{
  return scope->lookup(enc);
}

}

BOOST_PYTHON_MODULE(SymbolLookup)
{
  bpl::class_<SymbolSet> symbol_set("SymbolSet");

  bpl::class_<Scope, Scope *, boost::noncopyable> scope("Scope", bpl::no_init);
  scope.def("outer_scope", &Scope::outer_scope, bpl::return_value_policy<bpl::reference_existing_object>());
  scope.def("global_scope", &Scope::global_scope, bpl::return_value_policy<bpl::reference_existing_object>());
  scope.def("lookup", lookup_symbol);

  bpl::class_<Symbol, Symbol *, boost::noncopyable> symbol("Symbol", bpl::no_init);
  symbol.add_property("type",
		      bpl::make_function(&Symbol::type,
					 bpl::return_internal_reference<>()));
  // FIXME: find appropriate return value policy
  symbol.add_property("scope",
		      bpl::make_function(&Symbol::scope,
					 bpl::return_value_policy<bpl::reference_existing_object>()));
  // FIXME: should probably rename Symbol::ptree to Symbol::declarator, too
  symbol.add_property("declarator",
		      bpl::make_function(&Symbol::ptree,
					 bpl::return_value_policy<bpl::reference_existing_object>()));

  // symbols

  bpl::class_<VariableName, bpl::bases<Symbol>, VariableName *,
              boost::noncopyable> variable_name("VariableName", bpl::no_init);

  bpl::class_<ConstName, bpl::bases<Symbol>, ConstName *,
              boost::noncopyable> const_name("ConstName", bpl::no_init);
  // FIXME: should probably rename ConstName::defined to ConstName::is_defined, too
  const_name.def("is_defined", &ConstName::defined);
  const_name.add_property("value", &ConstName::value);

  bpl::class_<TypeName, bpl::bases<Symbol>, TypeName *,
              boost::noncopyable> type_name("TypeName", bpl::no_init);

  bpl::class_<TypedefName, bpl::bases<TypeName>, TypedefName *,
              boost::noncopyable> typedef_name("TypedefName", bpl::no_init);

  bpl::class_<ClassTemplateName, bpl::bases<Symbol>, ClassTemplateName *,
              boost::noncopyable> class_tmpl_name("ClassTemplateName", bpl::no_init);

  bpl::class_<FunctionName, bpl::bases<Symbol>, FunctionName *,
              boost::noncopyable> function_name("FunctionName", bpl::no_init);

  bpl::class_<FunctionTemplateName, bpl::bases<Symbol>, FunctionTemplateName *,
              boost::noncopyable> function_tmpl_name("FunctionTemplateName", bpl::no_init);

  bpl::class_<NamespaceName, bpl::bases<Symbol>, NamespaceName *,
              boost::noncopyable> namespace_name("NamespaceName", bpl::no_init);

  // scopes

  bpl::class_<TemplateParameterScope, bpl::bases<Scope>, TemplateParameterScope *,
              boost::noncopyable> template_parameter_scope("TemplateParameterScope", bpl::no_init);

  bpl::class_<LocalScope, bpl::bases<Scope>, LocalScope *,
              boost::noncopyable> local_scope("LocalScope", bpl::no_init);

  bpl::class_<FunctionScope, bpl::bases<Scope>, FunctionScope *,
              boost::noncopyable> function_scope("FunctionScope", bpl::no_init);

  bpl::class_<PrototypeScope, bpl::bases<Scope>, PrototypeScope *,
              boost::noncopyable> prototype_scope("PrototypeScope", bpl::no_init);

  bpl::class_<Class, bpl::bases<Scope>, Class *,
              boost::noncopyable> class_("Class", bpl::no_init);

  bpl::class_<Namespace, bpl::bases<Scope>, Namespace *,
              boost::noncopyable> namespace_("Namespace", bpl::no_init);

}
