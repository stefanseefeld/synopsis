//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_List_hh
#define _PTree_List_hh

#include <PTree/Node.hh>

namespace PTree
{

class List : public Node
{
public:
  List(Node *p, Node *q) : Node(p, q) {}
  bool is_atom() const { return false;}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  virtual void write(std::ostream &) const;
  virtual void print(std::ostream &, size_t, size_t) const;

  int Write(std::ostream&, int);  
protected:
  bool too_deep(std::ostream&, size_t) const;
  void print_encoded(std::ostream &, size_t, size_t) const;
};

}

#endif
