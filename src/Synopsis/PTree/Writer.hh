//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_PTree_Writer_hh_
#define Synopsis_PTree_Writer_hh_

#include <Synopsis/PTree.hh>
#include <sstream>

namespace Synopsis
{
namespace PTree
{

class Writer : private Visitor
{
public:
  Writer(std::ostream &os) : my_os(os) {}

  void write(Node const *n) { const_cast<Node *>(n)->accept(this);}
//   unsigned long write(Node const *);

private:

  virtual void visit(Atom *a) { my_os << string(a);}
  virtual void visit(List *l)
  {
    while(l)
    {
      Node *car = l->car();
      if(car) car->accept(this);
      my_os.put(' ');
      l = l->cdr();
    }
  }

  std::ostream &my_os;
};

inline std::string reify(Node const *p)
{
  if (!p) return "";
  else if (p->is_atom()) return string(static_cast<Atom const *>(p));
  else return string(static_cast<List const *>(p));
//   std::ostringstream oss;
//   Writer writer(oss);
//   writer.write(p);
//   return oss.str();
}

}
}

#endif
