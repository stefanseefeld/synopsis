//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/PTree/Node.hh>
#include <Synopsis/PTree/Encoding.hh>
#include <Synopsis/PTree/operations.hh>
#include <Synopsis/Buffer.hh>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cassert>
#include <stdexcept>

using namespace Synopsis;
using namespace PTree;

char const *Node::begin() const
{
  if (is_atom()) return position();
  else
  {
    for (Node const *p = this; p; p = p->cdr())
    {
      char const *b = p->car() ? p->car()->begin() : 0;
      if (b) return b;
    }
    return 0;
  }
}

char const *Node::end() const
{
  if (is_atom()) return position() + length();
  else
  {
    size_t n = PTree::length(static_cast<List const *>(this));
    while(n > 0)
    {
      char const *e = PTree::nth(static_cast<List const *>(this), --n)->end();
      if (e) return e;
    }    
    return 0;
  }
}

/*
Node *Iterator::pop()
{
  if(!ptree) return 0;
  else
  {
    Node *p = ptree->car();
    ptree = ptree->cdr();
    return p;
  }
}

bool Iterator::next(Node *& car)
{
  if(!ptree) return false;
  else
  {
    car = ptree->car();
    ptree = ptree->cdr();
    return true;
  }
}

Array::Array(size_t s)
{
  num = 0;
  if(s > 8)
  {
    size = s;
    array = new (GC) Node *[s];
  }
  else
  {
    size = 8;
    array = default_buf;
  }
}

void Array::append(Node *p)
{
  if(num >= size)
  {
    size += 8;
    Node **a = new (GC) Node *[size];
    memmove(a, array, size_t(num * sizeof(Node *)));
    array = a;
  }
  array[num++] = p;
}

Node *&Array::ref(size_t i)
{
  if(i < num) return array[i];
  else throw std::range_error("Array: out of range");
}

Node *Array::all()
{
  Node *lst = 0;
  for(int i = number() - 1; i >= 0; --i)
    lst = cons(ref(i), lst);
  return lst;
}
*/
