//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_TypeAnalysis_TypeRepository_hh_
#define Synopsis_TypeAnalysis_TypeRepository_hh_

#include <Synopsis/TypeAnalysis/Type.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolTable.hh>

namespace Synopsis
{
namespace TypeAnalysis
{

//. creates and remembers declared types.
class TypeRepository
{
public:
  static TypeRepository *instance() 
  {
    if (!instance_) instance_ = new TypeRepository;
    return instance_;
  }

  Type const *lookup(PTree::Encoding const &name, SymbolTable::Scope *scope);

  BuiltinType const *builtin(std::string const &name);
  Enum const *enum_(std::string const &name);
  Class const *class_(std::string const &name);
  Union const *union_(std::string const &name);
  CVType const *cvtype(Type const *type, CVType::Qualifier);
  Pointer const *pointer(Type const *type);
  Reference const *reference(Type const *type);
  Array const *array(Type const *type, unsigned long dim);
  Function const *function(Function::ParameterList const &params, Type const *retn);
  PointerToMember const *pointer_to_member(Type const *container, Type const *member);

private:
  struct Guard
  {
    ~Guard() { delete TypeRepository::instance_;}
  };

  TypeRepository();

  static TypeRepository *instance_;
  static Guard           guard;
};

}
}

#endif
