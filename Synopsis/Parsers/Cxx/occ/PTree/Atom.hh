//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_Atom_hh
#define _PTree_Atom_hh

#include <Lexer.hh>
#include <PTree/Node.hh>
#include <ostream>
#include <iterator>

namespace PTree
{

class Atom : public Node
{
public:
  Atom(const char *p, size_t l) : Node(p, l) {}
  Atom(const Token &t) : Node(t.ptr, t.length) {}
  bool is_atom() const { return true;}
  
  virtual void write(std::ostream &) const;
  virtual void print(std::ostream &, size_t, size_t) const;

  int Write(std::ostream&, int);
};

}

#endif
