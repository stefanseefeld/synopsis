//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_TypeAnalysis_Display_hh_
#define Synopsis_TypeAnalysis_Display_hh_

#include <Synopsis/TypeAnalysis/Visitor.hh>
#include <Synopsis/TypeAnalysis/Type.hh>
#include <Synopsis/TypeAnalysis/TypeRepository.hh>
#include <ostream>

namespace Synopsis
{
namespace TypeAnalysis
{

//. The Display class provides an annotated view of the types
//. for debugging purposes
class Display : private Visitor
{
public:
  Display(std::ostream &os);

  void display(Type const *);

private:
  virtual void visit(Type *);
  virtual void visit(BuiltinType *);
  virtual void visit(Enum *);
  virtual void visit(Class *);
  virtual void visit(Union *);

  virtual void visit(CVType *);
  virtual void visit(Pointer *);
  virtual void visit(Reference *);
  virtual void visit(Array *);
  virtual void visit(Function *);
  virtual void visit(PointerToMember *);

  virtual void visit(Parameter *);
  virtual void visit(Dependent *);

  std::ostream &my_os;
};

inline void display(Type const *type, std::ostream &os)
{
  Display d(os);
  d.display(type);
}

inline void display(TypeRepository const *repo, std::ostream &os)
{
  for (TypeRepository::siterator i = repo->sbegin(); i != repo->send(); ++i)
  {
//     os << i->first.unmangled() << ' ';
    display(i->second, os);
    os << '\n';
  }
}

}
}

#endif
