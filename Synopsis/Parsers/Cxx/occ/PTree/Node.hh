//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_Node_hh
#define _PTree_Node_hh

#include <PTree/GC.hh>
#include <PTree/Visitor.hh>
#include <Token.hh>
#include <ostream>
#include <iterator>

class Walker;
class TypeInfo;
class Encoding;

namespace PTree
{

class Node : public LightObject 
{
public:
  virtual ~Node() {}
  virtual bool is_atom() const = 0;
  virtual void accept(Visitor *visitor) = 0;

  //. return the start address of this Ptree in the buffer
  const char *begin() const;
  //. return the one-past-the-end address of this Ptree in the buffer
  const char *end() const;

  const char *position() const { return my_data.leaf.position;}
  size_t length() const { return my_data.leaf.length;}

  const Node *car() const { return my_data.nonleaf.child;}
  Node *car() { return my_data.nonleaf.child;}
  const Node *cdr() const { return my_data.nonleaf.next;}
  Node *cdr() { return my_data.nonleaf.next;}
  void set_car(Node *p) { my_data.nonleaf.child = p;}
  void set_cdr(Node *p) { my_data.nonleaf.next = p;}

  virtual Node *Translate(Walker*);
  virtual void Typeof(Walker*, TypeInfo&);

  virtual const char *encoded_type() const;
  virtual const char *encoded_name() const;

protected:
  //. used by Atom
  Node(const char *ptr, size_t len);
  //. used by List
  Node(Node *p, Node *q);

private:
  union 
  {
    struct 
    {
      Node *child;
      Node *next;
    } nonleaf;
    struct 
    {
      const char* position;
      int  length;
    } leaf;
  } my_data;
};

inline std::ostream &operator << (std::ostream &os, Node *p)
{
  // FIXME: this doesn't take into account any Ptree / AST modifications
  //        i.e. it just refers to the unmodified buffer
  //        Once we allow modifications to the tree we need to look up
  //        overlapping buffer replacements and inject them into the stream
  //        as appropriate
  std::copy(p->begin(), p->end(), std::ostream_iterator<char>(os));
  return os;
}

class Iterator : public LightObject 
{
public:
  Iterator(Node *p) { ptree = p;}
  Node *operator () () { return pop();}
  Node *pop();
  bool next(Node *&);
  void reset(Node *p) { ptree = p;}

  Node *get() { return ptree ? ptree->car() : 0;}
  Node *operator *() { return get();}
  Node *operator ++() { pop(); return get();}
  Node *operator ++(int) { return pop();}
  bool empty() { return ptree == 0;}
private:
  Node *ptree;
};

class Array : public LightObject 
{
public:
  Array(size_t = 8);
  size_t number() { return num;}
  Node *&operator [] (size_t index) { return ref(index);}
  Node *&ref(size_t index);
  void append(Node *);
  void clear() { num = 0;}
  Node *all();
private:
  size_t num, size;
  Node **array;
  Node *default_buf[8];
};

class Atom : public Node
{
public:
  Atom(const char *p, size_t l) : Node(p, l) {}
  Atom(const Token &t) : Node(t.ptr, t.length) {}
  bool is_atom() const { return true;}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class List : public Node
{
public:
  List(Node *p, Node *q) : Node(p, q) {}
  bool is_atom() const { return false;}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

}

#endif
