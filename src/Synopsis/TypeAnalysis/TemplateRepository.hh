//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_TypeAnalysis_TemplateRepository_hh_
#define Synopsis_TypeAnalysis_TemplateRepository_hh_

#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolTable.hh>
#include <Synopsis/TypeAnalysis/Type.hh>
#include <map>

namespace Synopsis
{
namespace TypeAnalysis
{

//. Store template declarations together with any (partial) specializations.
//. The symbol table only contains the primary template. Specializations
//. are stored here.
class TemplateRepository
{
public:
  //. Declare a specialization for the given template.
  void declare(SymbolTable::ClassTemplateName const *, PTree::ClassSpec const *);
  //. Declare a class scope associated with a template specialization.
  void declare_scope(SymbolTable::ClassTemplateName const *,
		     PTree::ClassSpec const *,
		     SymbolTable::Class *);
private:
  //. This needs a *lot* of work. For now only store *something* so
  //. we remember that a specialization exists.
  struct Specialization
  {
    Specialization(PTree::ClassSpec const *s = 0) : spec(s), scope(0) {}
    PTree::ClassSpec const *spec;
    SymbolTable::Class *scope;
  };
  typedef std::list<Specialization> SpecializationList;
  typedef std::map<SymbolTable::ClassTemplateName const *,
		   SpecializationList> Dictionary;

  Dictionary my_specializations;
};

}
}

#endif
