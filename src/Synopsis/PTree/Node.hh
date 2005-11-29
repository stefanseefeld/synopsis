//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_PTree_Node_hh_
#define Synopsis_PTree_Node_hh_

#include <Synopsis/PTree/GC.hh>
#include <Synopsis/PTree/Encoding.hh>
#include <Synopsis/PTree/Visitor.hh>
#include <Synopsis/Token.hh>
#include <ostream>
#include <iterator>
#include <stdexcept>

namespace Synopsis
{
namespace PTree
{

class List;

//. Abstract base class for parse trees. Its interface is minimal
//. to allow parse tree traversal with type recovery using visitors.
//. A node is either an atom, holding a single token, or a list,
//. similar to lisp's cons cell, referring to another node in its 'car'
//. member, and the next cons cell in its 'cdr' member.
class Node : public LightObject 
{
public:
  virtual ~Node() {}
  virtual bool is_atom() const = 0;
  virtual char const *position() const = 0;
  virtual size_t length() const = 0;
  virtual Node *car() const = 0;
  virtual List *cdr() const = 0;
  virtual void accept(Visitor *visitor) = 0;

  // FIXME: push these down to appropriate subclasses
  virtual Encoding encoded_type() const { return Encoding();}
  virtual Encoding encoded_name() const { return Encoding();}

  //. Return the start address of this Ptree in the buffer.
  virtual char const *begin() const = 0;
  //. Return the one-past-the-end address of this Ptree in the buffer.
  virtual char const *end() const = 0;
};

//. An Atom holds a single token from the input buffer.
class Atom : public Node
{
public:
  Atom(Token const &t) : my_position(t.ptr), my_length(t.length) {}
  Atom(char const *p, size_t l) : my_position(p), my_length(l) {}
  virtual bool is_atom() const { return true;}
  virtual char const *position() const { return my_position;}
  virtual size_t length() const { return my_length;}
  virtual Node *car() const { throw std::runtime_error("not a list");}
  virtual List *cdr() const { throw std::runtime_error("not a list");}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  virtual char const *begin() const { return position();}
  virtual char const *end() const { return position() + length();}
private:
  char const *my_position;
  size_t      my_length;
};

//. A List contains a node and a list.
struct List : public Node
{
public:
  List(Node *car, List *cdr) : my_car(car), my_cdr(cdr) {}
  virtual bool is_atom() const { return false;}
  virtual char const *position() const { throw std::runtime_error("not an atom");}
  virtual size_t length() const { throw std::runtime_error("not an atom");}
  virtual Node *car() const { return my_car;}
  virtual List *cdr() const { return my_cdr;}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  virtual char const *begin() const
  {
    char const *b = my_car ? my_car->begin() : 0;
    if (!b && my_cdr) b = my_cdr->begin();
    return b;
  }
  virtual char const *end() const 
  {
    char const *e = my_cdr ? my_cdr->end() : 0;
    if (!e && my_car) e = my_car->end();
    return e;
  }

  void set_car(Node *car) { my_car = car;}
  void set_cdr(List *cdr) { my_cdr = cdr;}

private:
  Node *my_car;
  List *my_cdr;
};

}
}

#endif
