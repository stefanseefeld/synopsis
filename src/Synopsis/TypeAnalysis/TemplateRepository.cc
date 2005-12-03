//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "TemplateRepository.hh"
#include <Synopsis/Trace.hh>

namespace Synopsis
{

namespace PT = PTree;
namespace ST = SymbolTable;

namespace TypeAnalysis
{

void TemplateRepository::declare(ST::ClassTemplateName const *primary,
				 PT::TemplateDeclaration const *spec)
{
  Trace trace("TemplateRepository::declare", Trace::SYMBOLLOOKUP);
  SpecializationList &specializations = my_specializations[primary];
  // Figure out what to do here.
  specializations.push_back(Specialization(spec));
}

void TemplateRepository::declare_scope(ST::ClassTemplateName const *primary,
				       PT::ClassSpec const *spec,
				       ST::Class *scope)
{
  Trace trace("TemplateRepository::declare_scope", Trace::SYMBOLLOOKUP);
  SpecializationList &specializations = my_specializations[primary];
  for (SpecializationList::iterator i = specializations.begin();
       i != specializations.end();
       ++i)
  {
//     if (i->spec == spec) i->scope = scope;
    break;
  }
}

}
}
