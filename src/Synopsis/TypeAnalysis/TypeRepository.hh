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

//. A repository of declared types, as well as type aliases.
class TypeRepository
{
  typedef std::map<PTree::Encoding, Type const *> Dictionary;
  typedef std::map<SymbolTable::Symbol const *, AtomicType const *> SDictionary;

public:
  typedef Dictionary::const_iterator iterator;
  typedef SDictionary::const_iterator siterator;

  static TypeRepository *instance() 
  {
    if (!instance_) instance_ = new TypeRepository;
    return instance_;
  }

  void declare(PTree::Encoding const &, SymbolTable::TypeName const *);
  void declare(PTree::Encoding const &, SymbolTable::ClassName const *);
  void declare(PTree::Encoding const &, SymbolTable::EnumName const *);

  Type const *lookup(PTree::Encoding const &name, SymbolTable::Scope *scope);

  BuiltinType const *builtin(std::string const &name);
  Enum const *enum_(std::string const &name);
  Class const *class_(Class::Kind kind, std::string const &name);
  Union const *union_(std::string const &name);
  CVType const *cvtype(Type const *type, CVType::Qualifier);
  Pointer const *pointer(Type const *type);
  Reference const *reference(Type const *type);
  Array const *array(Type const *type, unsigned long dim);
  Function const *function(Function::ParameterList const &params, Type const *retn);
  PointerToMember const *pointer_to_member(Type const *container, Type const *member);

  iterator begin() const { return my_types.begin();}
  iterator end() const { return my_types.end();}

  siterator sbegin() const { return my_declared_types.begin();}
  siterator send() const { return my_declared_types.end();}
private:
  struct Guard
  {
    ~Guard() { delete TypeRepository::instance_;}
  };
  friend struct Guard;

  TypeRepository();
  ~TypeRepository();

  static TypeRepository *instance_;
  static Guard           guard;

  //. Store types that need to be deleted.
  Dictionary my_types;
  //. Store declared types.
  SDictionary my_declared_types;
  //. Store type aliases for faster lookup.
  Dictionary my_aliases;
};

}
}

#endif
