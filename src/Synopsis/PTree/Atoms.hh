//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_PTree_Atoms_hh_
#define Synopsis_PTree_Atoms_hh_

#include <Synopsis/PTree/NodesFwd.hh>
#include <Synopsis/PTree/Node.hh>

namespace Synopsis
{
namespace PTree
{

class Literal : public Atom
{
public:
  Literal(Token const &tk) : Atom(tk), my_type(tk.type) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Token::Type type() const { return my_type;}
private:
  Token::Type my_type;
};

class CommentedAtom : public Atom
{
public:
  CommentedAtom(Token const &tk, List *c = 0) : Atom(tk), my_comments(c) {}
  // TODO: See Parser.cc for example where this is currently used.
  CommentedAtom(char const *p, size_t l, List *c = 0) : Atom(p, l), my_comments(c) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  List *get_comments() { return my_comments;}
  void set_comments(List *c) { my_comments = c;}
private:
  List *my_comments;
};

class Identifier : public CommentedAtom
{
public:
  Identifier(Token const &t, List *c = 0) : CommentedAtom(t, c) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class Keyword : public Atom
{
public:
  Keyword(Token const &t) : Atom(t) {}
  virtual Token::Type token() const = 0;
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

template <Token::Type t>
class KeywordT : public Keyword
{
public:
  KeywordT(Token const &tk) : Keyword(tk) {}
  virtual Token::Type token() const { return t;}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

}
}

#endif
