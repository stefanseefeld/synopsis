//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_Writer_hh
#define _PTree_Writer_hh

#include <PTree.hh>

namespace PTree
{

class Writer : public Visitor
{
public:
  Writer(std::ostream &os);

  unsigned long write(Node *);

private:

  virtual void visit(Atom *);
  virtual void visit(List *);
  virtual void visit(Brace *);

  void newline();

  std::ostream &my_os;
  size_t        my_indent;
  unsigned long my_lines;
};

std::string reify(const Node *);

}

#endif
