//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_PTree_Display_hh_
#define Synopsis_PTree_Display_hh_

#include <Synopsis/PTree.hh>

namespace Synopsis
{
namespace PTree
{

//. The Display class provides an annotated view of the ptree,
//. for debugging purposes
class Display : private Visitor
{
public:
  Display(std::ostream &os, bool encoded, bool typeinfo = false);

  void display(Node const *);

  virtual void visit(Atom *);
  virtual void visit(List *);
  // atoms...
  virtual void visit(DupAtom *);
  // ...lists...
  virtual void visit(Brace *);
  virtual void visit(Block *b) { visit(static_cast<Brace *>(b));}
  virtual void visit(ClassBody *b) { visit(static_cast<Brace *>(b));}
  virtual void visit(Declarator *l) { print_encoded(l);}
  virtual void visit(Name *l) { print_encoded(l);}
  virtual void visit(FstyleCastExpr *l) { print_encoded(l);}
private:
  void newline();
  bool too_deep();
  void print_encoded(List *);

  std::ostream &my_os;
  size_t        my_indent;
  size_t        my_depth;
  bool          my_encoded;
  bool          my_typeinfo;
};

inline void display(const Node *node, std::ostream &os,
		    bool encoded, bool typeinfo = false)
{
  Display d(os, encoded, typeinfo);
  d.display(node);
}

}
}

#endif
