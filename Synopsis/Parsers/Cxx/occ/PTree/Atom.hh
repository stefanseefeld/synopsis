//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_Atom_hh
#define _PTree_Atom_hh

#include <PTree/Node.hh>
#include <ostream>
#include <iterator>

namespace PTree
{

class Atom : public Node
{
public:
  Atom(const char *, size_t);
  Atom(const Token &);
  bool IsLeaf() const { return true;}
  
  virtual void write(std::ostream &) const;
  virtual void print(std::ostream &, size_t, size_t) const;

  int Write(std::ostream&, int);
};

}

#endif
